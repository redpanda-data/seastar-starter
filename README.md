# Seastar starter project

This project contains a small [Seastar](https://github.com/scylladb/seastar)
program and minimal cmake scaffolding. The example contains both coroutines
and continuation passing style uses of Seastar.

# Building

See the section below on requirements for specific environments that are known to work.

Install dependencies:

```bash
$> git submodule update --init --recursive
$> seastar/install-dependencies.sh
$> apt-get install -qq ninja-build clang
```

Configure and build:

```
$> cmake -Bbuild -S. -GNinja
$> ninja -C build
```

Or if you need to specify a non-default compiler:

```
$> CC=clang CXX=clang++ cmake -Bbuild -S. -GNinja
$> ninja -C build
```

# Requirements

A compiler that supports at least C++17 is required. Some examples known to work:

* Ubuntu 22.04. The default GCC version is new enough to work. This should be installed by default with the instructions above. This is also the combination that runs in our CI. See [.github/workflows/build.yml](.github/workflows/build.yml). Both clang-14 and clang-15 have issues on Ubuntu 22.04.
* Ubuntu 23.10. Both the default GCC 13.2.0 and Clang 16 are known to work out of the box.
* Fedora 38 and newer are known to work.

# Running

The sample program splits an input file into chunks. Each core reads a subset of
the input file into memory and then writes this subset out to a new file. The
size of the per-core subset may be larger than memory, in which case more than
one subset per core will be generated.

First, generate some data. This command will generate around 200mb of data.

```
dd if=/dev/zero of=input.dat bs=4096 count=50000
```

Next, invoke the program. Here we limit the total system memory to 500 MB, use 5
cores, and then we further limit memory to 1% of the amount available on each
core. Each command line argument is optional except the input file. If the
amount of memory or the number of cores are not specified then the program will
try to use all of the resources available.

```
$ build/main --input input.dat -m500 -c5 --memory-pct 1.0
```

The program should output a summary on each core about the data it is
responsible for, and then once per second a per-core progress is printed.

```
INFO  2024-01-13 13:10:14,214 [shard 0] splitter - Processing 10000 pages with index 0 to 9999
INFO  2024-01-13 13:10:14,214 [shard 1] splitter - Processing 10000 pages with index 10000 to 19999
INFO  2024-01-13 13:10:14,214 [shard 3] splitter - Processing 10000 pages with index 30000 to 39999
INFO  2024-01-13 13:10:14,214 [shard 2] splitter - Processing 10000 pages with index 20000 to 29999
INFO  2024-01-13 13:10:14,214 [shard 4] splitter - Processing 10000 pages with index 40000 to 49999
INFO  2024-01-13 13:10:14,214 [shard 0] splitter - Progress: 0.0 0.0 0.0 0.0 0.0
INFO  2024-01-13 13:10:15,214 [shard 0] splitter - Progress: 54.5 54.3 55.2 53.8 53.6
INFO  2024-01-13 13:10:16,215 [shard 0] splitter - Progress: 100.0 100.0 100.0 100.0 100.0
```

After the program exists there should be a number of chunk files on disk. The
chunk file format is `chunk.<core-id>.<chunk-id>`.

```
$ ls -l chunk*
-rw-r--r--. 1 user user 331776 Jan 13 13:10 chunk.core-0.0
-rw-r--r--. 1 user user 331776 Jan 13 13:10 chunk.core-0.1
-rw-r--r--. 1 user user 331776 Jan 13 13:10 chunk.core-0.10
-rw-r--r--. 1 user user 331776 Jan 13 13:10 chunk.core-0.100
-rw-r--r--. 1 user user 331776 Jan 13 13:10 chunk.core-0.101
-rw-r--r--. 1 user user 331776 Jan 13 13:10 chunk.core-0.102
-rw-r--r--. 1 user user 331776 Jan 13 13:10 chunk.core-0.103
```

# Resources

* [The Seastar tutorial](https://github.com/scylladb/seastar/blob/master/doc/tutorial.md)
* [ScyllaDB](https://github.com/scylladb/scylla) is a large project that uses Seastar.
* [CMake tutorial](https://cmake.org/cmake-tutorial/)

# Testing

This project uses a GitHub action to run the same set of instructions as above. Please
see [.github/workflows/build.yml](.github/workflows/build.yml) for reference.
