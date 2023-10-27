# VCPP
A simple interpreter of a language create by me. This language contains the shortcomings of both C++ and Java. !!!GC is included.

## How to Build
After you download the sources, open the directory of this project in ``visual studio code``(``vscode`` in short) in Linux or in subsystem of Linux. Make sure your vscode has already had the extensions listed below:
1. C++
2. CMake
3. CMake Tools

And there are some necessary commands:
``g++, gcc, gdb, ninja, cmake, make``

You can use the building program of cmake extension or $\texttt{F5}$ to build the one of ``Interpreter`` and ``Runtime``

## Warning

I haven't tried to build this project in ``Windows`` or ``MacOS`` , so you may face some problems when building this project in these platform.

## Directory Structure

the files in subdirectory named ``Interpreter`` are the source files for the Interpreter of ``VCPP``, while those in ``Runtime`` are for the Runtime of ``VCPP`` .

I provide some ``VCPP`` files for testing in ``TestEnv``, which is also the default test environment for the interpreter and the runtime.