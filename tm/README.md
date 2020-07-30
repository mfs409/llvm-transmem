# TM

Note: This folder contains a subset of the code in the `ptm_tm` folder.
Specifically, it removes all support for persistent transactional memory from
the `libs` folder.  This reduction makes it easier to assess the implementation
overhead of providing support for transactional memory in C++.  The
documentation in this file has not been updated to reflect these changes.

Transactional memory (TM) broadly refers to a concurrency mechanism in which
programmers annotate regions of code that require atomicity, and then a run-time
system, often augmented by hardware support or aided by compiler
instrumentation, assumes responsibility for making sure that when those regions
of code are simultaneously executed by different threads, their memory accesses
do not result in conflicts or data races.

While the GNU Compiler Collection already provides rich support for TM, through
an implementation of the [2015 C++ TM Technical Specification
(TMTS)](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4514.pdf), the
TMTS is widely viewed to be too complicated, and as a result, no other vendors
have implemented it in their compilers.  Furthermore, the complexities of the
TMTS make it difficult to repurpose the TM support in GCC.

The code in this repository has three goals:

1. To present fully functional, simple, effective implementations of TM that are
   easier to implement and use than what is proposed in the TMTS.
2. To enable researchers to customize the TM implementation much more easily
   than is possible in GCC.
3. To provide the authors with a common public repository for sharing research
   artifacts related to their published work on TM

Note that these goals have distinct audiences.  The code in this repository aims
to serve all of them from as few branches as possible.  Sometimes, this means
the build mechanisms are slightly more complex than necessary for any single
audience.  We have tried our best to document these complexities, and apologize
for any confusion that may result.  We encourage those with questions to use the
*issues* feature.

## High Level Design

Below, we briefly describe the high-level design of our system.  For more
details, please see our [TACO 2019
paper](http://www.cse.lehigh.edu/~spear/papers/zardoshti-taco-2019.pdf):

"Simplifying Transactional Memory Support in C++", by PanteA Zardoshti, Tingzhe
Zhou, Pavithra Balaji, Michel L. Scott, and Michael Spear. ACM Transactions on
Architecture and Code Optimization (TACO), 2019.

For those interested specifically in the persistent TM support, please see our
[PACT 2019 paper](http://sss.cse.lehigh.edu/files/pubs/CR-PACT-2019.pdf):

"Optimizing Persistent Memory Transactions", by PanteA Zardoshti, Tingzhe Zhou,
Yujie Liu, and Michael Spear. 28th International Conference on Parallel
Architectures and Compilation Techniques (PACT), 2019.

### The Programmer's API

For the purposes of standardization in C++, we consider two ways to express
transactions.  Neither of these approaches requires changes to the C++ grammar,
and hence both can be used with *any* modern C++ compiler.

Note that the two approaches below have subtle differences in terms of run-time
overhead and the complexity of compiler support.  Approaches may also introduce
subtle limitations on what transactional code can do (most notably, access to
`varargs`).  For details, see the recent proposal [Transactional Memory Lite
Support in
C++](http://open-std.org/JTC1/SC22/WG21/docs/papers/2019/p1875r0.pdf).

Also, please note that this presentation glosses over a few implementation
artifacts (such as our workaround for LLVM's lack of support for function
annotations on constructors).

**Option 1: Lambdas**: To use transactions with the lambda-based syntax, one
would type code in the following manner:

```C++
tm_exec([&] { /* transaction body goes here */ })
```

**Option 2: RAII**: To use transactions with the RAII-based syntax, one would
type code in the following manner:

```C++
{ tm_scope tm(); /* transaction body goes here */ }
```

**Support for C**: We have a mechanism that works for C programs.  For details,
see the paper, or `common/tm_api.h`

**Notice**: In our implementations, we need greater flexibility in order to be
compatible with the variety of TM algorithms that we offer.  To that end, we use
macros instead of the names that appear above.  See `common/tm_api.h` for more
detail.

### The TM Library

The purpose of a TM library is to monitor the execution of transactions at run
time, so that conflicting memory accesses between transactions can be caught
before they lead to incorrect program behavior.

There are three common ways to achieve this goal:

1. No concurrency.  As a degenerate implementation, one could protect all
   transactions with a single, global, reentrant mutex.  This can actually
   scale, if transactions are rare and small.
2. Hardware TM.  By employing hardware TM support, such as Intel TSX, one can
   have the hardware monitor transaction execution, and roll back transactions
   when conflicts would otherwise manifest.
3. Software and Hybrid TM.  When the compiler instruments each load and store of
   each transaction, so that each becomes a call to a library, that library can
   monitor the execution of transactions, manage rollback, etc.

Note that options #1 and #2 do not require compiler support.  Also note that all
three options are compatible with both programmer APIs.

### The Compiler Plugin

The purpose of the compiler plugin is to instrument the bodies of transactions,
so that every load and store within the dynamic control flow graph of the
transaction becomes a call to the TM library.  This involves discovery of the
full control flow graph.  Sometimes, that discovery is impossible, due to
separate compilation.  We allow the programmer to annotate functions in order to
aid with separate compilation.  We also allow the TM libraries to fall back to
an "irrevocable" mode, in which transactions serialize when they cannot find an
instrumented version of a function they must call.

It is occasionally the case (for example in certain persistent TM systems) that
only the *stores* within a transaction need instrumentation.  We provide two
versions of our plugin, in order to handle this relaxed requirement.

## Benchmarks

We provide a variety of standard benchmarks, such as the STAMP benchmark suite,
data structure microbenchmarks, and the memcached application.

## Getting Started

Please use the provided `Dockerfile` to ensure you have a reasonable build
environment.  Our plugin has been tested with LLVM-4.0 through LLVM-10.0.  The
current code is only tested with LLVM-10.0 and Ubuntu 20.04.

Individual components can be built by entering into one of the main directories
(plugin, libs, benchmarks) and typing `make`.  Typing `make` from the top-level
directory will traverse into each subdirectory and build everything.

Note that typing `make` in the benchmark folder will produce one version of each
benchmark for each TM algorithm in the libs folder, times each of each of the
programmer APIs.  Four additional versions will be produced, corresponding to
the use of each of the programmer APIs for "single-lock-only" or "hardware TM"
libraries.  These additional versions show the performance that can be expected
when a compiler designer opts to provide minimal (e.g., library-only) support
for TM.

## Persistence

Please note that the support for persistent TM is experimental, and intended
only for studying overheads and the impact of optimizations.  We strongly
discourage the use of our persistent TM code for anything other than research.
In particular, we are not providing code in this public release for interacting
with a persistent allocator.
