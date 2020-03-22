# Re-implementation of Lock Free Search Trees

## Building and Running

This project is built with CMake version 3.0.0+ using g++ with support for C++11 and posix threads. Ensure the correct versions of `cmake` and `g++` are installed and added to the system path before continuing.

1. Navigate to the `build/` directory in the terminal.
2. Run `cmake -G "MinGW Makefiles" ..` to configure the project. This only needs to be done once.
2. Run `cmake --build .` to build the project
3.  Run `./lfca` to execute the test program, or `./TEST` To execute the unit tests
