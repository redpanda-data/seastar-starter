#include <seastar/core/app-template.hh>
#include <seastar/core/sharded.hh>
#include <seastar/core/sstring.hh>
#include <seastar/core/reactor.hh>

#include <chrono>
#include <iostream>

// the speak service runs on every core (see `seastar::sharded<speak_service>`
// below). when the `speak` method is invoked, it returns a message tagged with
// the core on which the method was invoked.
class speak_service final {
public:
    speak_service(const seastar::sstring& msg)
      : _msg(msg) {
    }

    seastar::sstring speak() {
        std::stringstream ss;
        ss << "msg: \"" << _msg << "\" from core "
           << seastar::this_shard_id();
        return ss.str();
    }

    seastar::future<> stop() {
        return seastar::make_ready_future<>();
    }

private:
    seastar::sstring _msg;
};

int main(int argc, char** argv) {
    seastar::sharded<speak_service> speak;

    seastar::app_template app;
    {
        namespace po = boost::program_options;
        app.add_options()(
          "msg",
          po::value<seastar::sstring>()->default_value("default-msg"),
          "msg");
    }

    return app.run(argc, argv, [&] {
        seastar::engine().at_exit([&speak] { return speak.stop(); });

        auto& opts = app.configuration();
        auto msg = opts["msg"].as<seastar::sstring>();

        return speak.start(msg).then([&speak] {
            // sharded<>::map will run the provided lambda on each core. in this
            // case, the speak method of the service is invoked and the messaes
            // from each core are printed to stdout.
            return speak.map([](auto s) { return s.speak(); })
              .then([](auto msgs) {
                  for (auto msg : msgs) {
                      std::cout << msg << std::endl;
                  }
                  return seastar::make_ready_future<int>(0);
              });
        });
    });
}
