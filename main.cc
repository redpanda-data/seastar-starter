#include <seastar/core/app-template.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/fstream.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sharded.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/sstring.hh>
#include <seastar/util/log.hh>

#include <sys/wait.h>

#include <chrono>
#include <iostream>

static seastar::logger lg("splitter");

class file_splitter final {
    static constexpr size_t page_size = 4096;

public:
    file_splitter(std::filesystem::path path, double memory_pct)
      : path_(std::move(path))
      , memory_pct_(memory_pct) {
    }

    seastar::future<> start() {
        file_ = co_await seastar::open_file_dma(
          path_.string(), seastar::open_flags::ro);

        const auto size = co_await file_.size();

        /*
         * limit input size to be a multiple of page size for simpler math.
         */
        if (size % page_size != 0) {
            throw std::runtime_error(fmt::format(
              "Input file size {} must be a multiple of page size {}",
              size,
              page_size));
        }

        const auto total_pages = size / page_size;
        const auto pages_per_core = total_pages / seastar::smp::count;
        start_page_ = pages_per_core * seastar::this_shard_id();

        /*
         * the end page for the core with the largest id is adjusted to
         * account for an uneven division of pages among cores.
         */
        end_page_ = [&] {
            if (seastar::this_shard_id() == (seastar::smp::count - 1)) {
                return total_pages - 1;
            }
            return start_page_ + pages_per_core - 1;
        }();

        lg.info(
          "Processing {} pages with index {} to {}",
          end_page_ - start_page_ + 1,
          start_page_,
          end_page_);

        /*
         * invokes `run()` in the background. in order to be able to synchronize
         * with the background fiber running it is started under a `gate` which
         * can be used to wait until the background fiber finishes, which is
         * done in `stop()`.
         */
        std::ignore = seastar::with_gate(gate_, [this] { return run(); });
    }

    seastar::future<> stop() {
        co_await gate_.close();
        co_await file_.close();
    }

    double progress() const {
        if (gate_.is_closed()) {
            return 100.0;
        }
        return (static_cast<double>(curr_page_ - start_page_)
                / (end_page_ - start_page_))
               * 100.0;
    }

private:
    seastar::future<> run() {
        const auto pages_memory_limit = static_cast<size_t>(
          (seastar::memory::stats().total_memory() * memory_pct_) / page_size);

        /*
         * read up to pages_memory_limit pages worth of data and then write the
         * data to a new chunk file. repeat until all pages have been processed.
         */
        size_t chunk = 0;
        seastar::chunked_fifo<seastar::temporary_buffer<char>> pages;
        for (curr_page_ = start_page_; curr_page_ < end_page_; ++curr_page_) {
            /*
             * allocate a page and read it from the file. the dma-family of I/O
             * interfaces require aligned memory, size, and file offsets.
             */
            auto buf = seastar::temporary_buffer<char>::aligned(
              page_size, page_size);

            const auto size = co_await file_.dma_read(
              curr_page_ * page_size, buf.get_write(), page_size);

            /*
             * check for a short read. we don't handle it, but retrying or
             * reading the remainder would be reasonable. see also
             * seastar::file::dma_read_bulk.
             */
            if (size != page_size) {
                throw std::runtime_error(fmt::format(
                  "Short read with size {} != {} occurred at offset {}",
                  size,
                  page_size,
                  curr_page_ * page_size));
            }

            pages.push_back(std::move(buf));

            /*
             * keep reading until we've reached the memory limit or last page.
             */
            if (
              pages.size() < pages_memory_limit
              && curr_page_ != (end_page_ - 1)) {
                continue;
            }

            auto pages_to_write = std::exchange(pages, {});

            /*
             * open a file for this chunk which this core owns
             */
            const auto filename = fmt::format(
              "chunk.core-{}.{}", seastar::this_shard_id(), chunk++);

            auto output = co_await seastar::open_file_dma(
              filename,
              seastar::open_flags::create | seastar::open_flags::truncate
                | seastar::open_flags::wo);

            /*
             * stream the pages to the output chunk file.
             */
            auto ostream = co_await seastar::make_file_output_stream(
              std::move(output));

            lg.debug(
              "Dumping {} pages to file {}. Page buffering limit {}",
              pages_to_write.size(),
              filename,
              pages_memory_limit);

            for (auto& page : pages_to_write) {
                co_await ostream.write(page.get(), page.size());
            }

            co_await ostream.flush();
            co_await ostream.close();
        }
    }

    std::filesystem::path path_;
    double memory_pct_;
    seastar::gate gate_;
    seastar::file file_;
    size_t start_page_{0};
    size_t end_page_{0};
    size_t curr_page_{0};
};

/*
 * Monitor the progress of the splitter. This method expects that splitter has
 * already been started.
 */
static seastar::future<> monitor(seastar::sharded<file_splitter>& splitter) {
    while (true) {
        /*
         * query the progress for the splitter on each core
         */
        const auto progress = co_await splitter.map(
          [](file_splitter& splitter) { return splitter.progress(); });

        lg.info("Progress: {:.1f}", fmt::join(progress, " "));

        const auto all_done = std::all_of(
          progress.begin(), progress.end(), [](auto p) { return p == 100.0; });
        if (all_done) {
            break;
        }

        co_await seastar::sleep(std::chrono::seconds(1));
    }
}

int main(int argc, char** argv) {
    seastar::sharded<file_splitter> splitter;

    seastar::app_template app;
    {
        namespace po = boost::program_options;

        /*
         * --input <file> to be processed.
         */
        app.add_options()(
          "input", po::value<seastar::sstring>()->required(), "input file");

        /*
         * --memory-pct <pct> is the percentage of the available memory to use
         *  for buffering. up to this much data will be read into memory before
         *  dumping out an intermediate file.
         */
        app.add_options()(
          "memory-pct",
          po::value<double>()->default_value(20.0),
          "percent of shard memory to use");
    }

    return app.run(argc, argv, [&] {
        seastar::engine().at_exit([&splitter] { return splitter.stop(); });

        auto& opts = app.configuration();
        const auto input = std::filesystem::path(
          opts["input"].as<seastar::sstring>());
        const auto memory_pct = opts["memory-pct"].as<double>() / 100.0;

        return splitter.start(input, memory_pct).then([&] {
            return splitter.invoke_on_all(&file_splitter::start)
              .then([&splitter] { return monitor(splitter); })
              .then([] { return seastar::make_ready_future<int>(0); });
        });
    });
}
