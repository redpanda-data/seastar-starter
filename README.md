# Seastar starter project

This project contains a small [Seastar](https://github.com/scylladb/seastar)
program and minimal cmake scaffolding for building both the sample program and
seastar as a dependency.

# Getting started

```bash
git submodule update --init
sudo ./install-deps.sh
cmake .
make
./main
```

Running the program `./main --msg sup` should produce output similar to:

```
msg: "sup" from core 0
msg: "sup" from core 1
msg: "sup" from core 2
msg: "sup" from core 3
msg: "sup" from core 4
msg: "sup" from core 5
msg: "sup" from core 6
msg: "sup" from core 7
msg: "sup" from core 8
msg: "sup" from core 9
msg: "sup" from core 10
msg: "sup" from core 11
```

# Resources

* [The Seastar tutorial](https://github.com/scylladb/seastar/blob/master/doc/tutorial.md)
* [ScyllaDB](https://github.com/scylladb/scylla) is a large project that uses Seastar.
* [CMake tutorial](https://cmake.org/cmake-tutorial/)

# Testing

Tested on

* Fedora 30

See `test.sh` for details.
