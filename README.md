# Torch

Simple way to build run, build and use Tor.

## Building

You need to build tor and all dependencies, it usually boils down to building:
 - xz
 - zstd
 - libevent
 - zlib
 - openssl
 - libtor (make libtor.a in tor, make sure to have `include/tor/feature/api/tor_api.h`)

then it is as simple as running the following:

```
$ mkdir build && cd build
$ cmake .. -DTOR_BUILD_DIR=<path to directory where packages are built>
$ make
```
