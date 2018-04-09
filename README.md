NUClear [![Build Status](https://travis-ci.org/Fastcode/NUClear.svg?branch=master)](https://travis-ci.org/Fastcode/NUClear)
=======

NUClear is a C++ software framework designed to aid in the development of real time modular systems.
It is built from a set of template metapgrograms that control the flow of information through the system.

It is highly extensible and provides several attachment points to develop new DSL keywords if needed.

These metaprograms reduce the cost of routing messages between modules resulting in a much faster system.

For help getting started check the [wiki](https://github.com/Fastcode/NUClear/wiki)

If you're starting a new project using NUClear the [NUClear Roles system](https://github.com/Fastcode/NUClearRoles) is highly recommended

## Installing
NUClear uses CMake as its build tool.

For the lazy
```bash
git clone https://github.com/Fastcode/NUClear.git
cd NUClear
mkdir build
cd build
cmake .. -DBUILD_TESTS=OFF
make install
```

### Dependencies
* g++ 4.9, clang (with c++14 support) or Visual Studio 2015
* cmake 2.8.10
* [Catch Unit Testing Framework](https://github.com/philsquared/Catch) for building tests
