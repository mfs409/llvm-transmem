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

Also, our PACT paper `["Optimizing Persistent Memory Transactions", by PanteA Zardoshti, Tingzhe Zhou, and Micha\ el Spear. International Conference on Parallel Architectures and Compilation Techniques(PACT), 2019.]`

## Build Instructions
1. Build the LLVM pass, libraries and benchmarks
<pre>
    ./compile.sh
</pre>

2. Run benchmarks 
<pre>
    ./compile.sh all
</pre>

3. Compile an example to a .o file
<pre>
    cd examples
    clang++ -Xclang -load -Xclang ../plugin/build/libtmplugin.so -c -I ../libs/ exampleX.cc
</pre>

4. To see the output, use these commands
<pre>
    clang++ exampleX.o -o exampleX
    ./exampleX
</pre>

5. You can read assembly code and symbol table with these commands
<pre>
    llvm-nm example.o
    llvm-objdump -t -r exampleX.o
    llvm-objdump -d exampleX.o
</pre>

6. To read bitcode
<pre>
    clang++ -emit-llvm -c exampleX.cc -o exampleX.bc
    llvm-dis exampleX.bc
    cat exampleX.ll
</pre>

7. To generate graph
<pre>
    ./generate.sh
</pre>

## Add New Benchmarks
1. Include the tm library
<pre>
    #include "include/tm.h"
 </pre>
 
2. Replace every lock with TX_BEGIN and TX_END keywords
<pre>
    lock();
    shared_counter++;
    unlock();
</pre>
    
<pre>
    TX_BEGIN {
    shared_counter++;
    } TX_END;
</pre>

3. Compile the benchmark to a .o file
<pre>
    clang++ -Xclang -load -Xclang ../plugin/build/libtmplugin.so -c -I ../libs/ benchmarkX.cc
</pre>

4. Link the benchmark with your library
<pre>
    g++  benchmarkX  benchmarkX.o ../libs/./obj64/$libname.o
</pre>
