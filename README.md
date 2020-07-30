# LLVM-TRANSMEM

The `llvm-transmem` repository holds public versions of the following software
artifacts:

1. A system for implementing transactional memory in C++, as discussed in
   <http://wg21.link/p1875>.  This system is in the `tm` folder.  It extends
   `["Simplifying Transactional Memory Support in C++", by PanteA Zardoshti,
   Tingzhe Zhou, Pavithra Balaji, Michel L. Scott, and Michael Spear. ACM
   Transactions on Architecture and Code Optimization (TACO), 2019.]`.

2. An extension of the above system, which adds support for persistent
   transactional memory.  This system is in the `ptm_tm` folder.  It is based on
   `["Optimizing Persistent Memory Transactions", by PanteA Zardoshti, Tingzhe
   Zhou, and Michael Spear. International Conference on Parallel Architectures
   and Compilation Techniques (PACT), 2019.]`

The code in this repository is provided as-is, with no warranty.  It is intended
for research, or as a proof of concept.  The authors make no guarantees about it
being production-ready.

This code has been tested with versions of LLVM from 5.0 through 10.0.  It is
currently known to work within the container described by the provided
`Dockerfile`.  That file represents an Ubuntu 20.04 image with LLVM 10.0.  No
effort has been made to retain backwards compatibility.

## Getting Started

When getting started, we encourage use of the included Docker container
definition.  All of our code and benchmarks should compile and run without
trouble using this container definition.

If your use of this software leads to a research publication about transactional
memory, please cite our TACO paper `["Simplifying Transactional Memory Support
in C++", by PanteA Zardoshti, Tingzhe Zhou, Pavithra Balaji, Michel L. Scott,
and Michael Spear. ACM Transactions on Architecture and Code Optimization
(TACO), 2019.]`

If your use of this software leads to a research publication about persistence,
please cite our PACT paper `["Optimizing Persistent Memory Transactions", by
PanteA Zardoshti, Tingzhe Zhou, and Michael Spear. International Conference on
Parallel Architectures and Compilation Techniques(PACT), 2019.]`

## Build Instructions

1. To build the LLVM pass, libraries and benchmarks, type `make`
2. To run benchmarks, please follow advice from the publications that introduced
   those benchmarks.
3. To compile an example to a .o file, type `clang++ -Xclang -load -Xclang {path_to_plugin}/libtmplugin.so {other flags} {file.cc}`

## Programming with the Plugin

The Makefiles and examples in the benchmark folders show how to use the Lambda
and RAII APIs to write transactional programs.  Below is a brief overview:

* Include the tm library: `#include <common/tm_api.h>`

* Replace every lock with TX_BEGIN and TX_END keywords or with a TX_RAII call:

```c++
lock();
shared_counter++;
unlock();
```

```c++
TX_BEGIN {
    shared_counter++;
} TX_END;
```

```c++
{
    TX_RAII;
    shared_counter++;
}
```

* Compile:  `clang++ -Xclang -load -Xclang {path_to_plugin}/libtmplugin.so {other flags} -c {file.cc} -o {file.o}`

* Link: `g++ -o {executable_name} {file.o} {other flags} {path_to_libs}/{tm_algorithm}.o`
