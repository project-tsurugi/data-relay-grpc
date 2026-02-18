# data relay gRPC service

## Requirements

* CMake `>= 3.16`
* C++ Compiler `>= C++17`
* and see *Dockerfile* section

```sh
# retrieve third party modules
git submodule update --init --recursive
```

### Dockerfile

```dockerfile
FROM ubuntu:22.04

RUN apt update -y && apt install -y git build-essential cmake ninja-build libgoogle-glog-dev libgflags-dev protobuf-compiler protobuf-c-compiler libprotobuf-dev protobuf-compiler-grpc libgrpc++-dev pkg-config openssl
```
optional packages:

* `doxygen`
* `graphviz`
* `clang-tidy-14`

### Install modules

## How to build

```sh
mkdir -p build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

available options:
* `-DCMAKE_INSTALL_PREFIX=<installation directory>` - change install location
* `-DCMAKE_PREFIX_PATH=<installation directory>` - indicate prerequisite installation directory
* `-DCMAKE_IGNORE_PATH="/usr/local/include;/usr/local/lib/"` - specify the libraries search paths to ignore. This is convenient if the environment has conflicting version installed on system default search paths. (e.g. gflags in /usr/local)
* `-DBUILD_TESTS=OFF` - build test programs
* `-DBUILD_DOCUMENTS=OFF` - build documents by doxygen
* `-DBUILD_STRICT=OFF` - don't treat compile warnings as build errors
* `-DUSE_GRPC_CONFIG=ON` - use gRPC CMake Config mode
* for debugging only
  * `-DENABLE_SANITIZER=OFF` - disable sanitizers (requires `-DCMAKE_BUILD_TYPE=Debug`)
  * `-DENABLE_UB_SANITIZER=ON` - enable undefined behavior sanitizer (requires `-DENABLE_SANITIZER=ON`)
  * `-DSMOKE_TEST_SUPPORT=ON` - enable smoke test support.

### install

```sh
cmake --build . --target install
```

### run tests

```sh
ctest -V
```

### generate documents

```sh
cmake --build . --target doxygen
```

### Recompile time saving with ccache

You can save re-compile time using [ccache](https://ccache.dev), which caches the compiled output and reuse on recompilation if no change is detected.
This works well with jogasaki build as jogasaki build has dependencies to many components and translation units are getting bigger.

1. Install ccache
```
apt install ccache
```
2. add `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` to cmake build option.

First time build does not change as it requires building and caching all artifacts into cache directory, e.g. `~/.ccache`. When you recompile, you will see it finishes very fast.
Checking `ccache -s` shows the cache hit ratio and size of the cached files.

## License

[Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)

