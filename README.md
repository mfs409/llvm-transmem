# LLVM-TRANSMEM

The llvm-transmem repository holds public versions of the following software
artifacts:

* plugin: Source code for the llvm plugin itself.  This code has been tested with LLVM 4, 5, and 6.  It does not require LLVM sources, nor does it require changes to the clang/clang++ frontend.

* libs: Source code for several transactional memory library implementations.

* tests: Test cases for verifying that the plugin correctly instruments code.

* ubench: A set of data structure microbenchmarks.

* benchmarks: A set of larger-scale benchmarks

When getting started, we encourage use of the included Docker container definition.  All of our code, to include benchmarks, should compile and run without trouble using this container definition.

If your use of this software leads to a research publication, please cite our TACO paper `["Simplifying Transactional Memory Support in C++", by PanteA Zardoshti, Tingzhe Zhou, Pavithra Balaji, Michel L. Scott, and Michael Spear. ACM Transactions on Architecture and Code Optimization (TACO), 2019.]`
