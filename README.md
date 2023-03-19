# Seastar starter project

This project contains a small [Seastar](https://github.com/scylladb/seastar)
program and minimal cmake scaffolding. The example contains both coroutines
and continuation passing style uses of Seastar.

# Getting started

Install dependencies (assuming a recent verison of Ubuntu such as 22.04):

```bash
git submodule update --init --recursive
seastar/install-dependencies.sh
apt-get install -qq ninja-build clang
```

Configure and build:

```
CXX=clang++ CC=clang cmake -Bbuild -S. -GNinja
ninja -C build
```

Run the example program `./main --msg sup`:

```
build/main --msg sup
```

Similar output to the following should be produced:

```
INFO  2022-10-24 20:18:56,540 [shard 0] speak-log - Processed speak request
INFO  2022-10-24 20:18:57,540 [shard 1] speak-log - Processed speak request
INFO  2022-10-24 20:18:58,540 [shard 2] speak-log - Processed speak request
INFO  2022-10-24 20:18:59,540 [shard 3] speak-log - Processed speak request
INFO  2022-10-24 20:19:00,540 [shard 4] speak-log - Processed speak request
INFO  2022-10-24 20:19:01,540 [shard 5] speak-log - Processed speak request
msg: "sup" from core 0
msg: "sup" from core 1
msg: "sup" from core 2
msg: "sup" from core 3
msg: "sup" from core 4
msg: "sup" from core 5
```

# Resources

* [The Seastar tutorial](https://github.com/scylladb/seastar/blob/master/doc/tutorial.md)
* [ScyllaDB](https://github.com/scylladb/scylla) is a large project that uses Seastar.
* [CMake tutorial](https://cmake.org/cmake-tutorial/)

# Testing

This project uses a GitHub action to run the same set of instructions as above. Please
see [.github/workflows/build.yml](.github/workflows/build.yml) for reference.
