# llvm-tm-support/libs

The llvm-tm-support/libs folder is the home for all of the TM and PTM algorithms
that are available for use in our system.  Note that these algorithms include
traditional STM algorithms, Hybrid algorithms that try to use hardware TM
support when it is available, and algorithms for performing transactions over
persistent memory.

## Structure of a TM implementation

In order to share code as much as possible, and to support as many algorithms
as possible without creating too much clutter, we use the following strategy.
We begin with an algorithm category, typically based on fundamental
synchronization decisions, such as the type of metadata used to mediate
concurrent access to a location and how updates to the shared memory are
managed.  We parameterize this implementation based on implementation
options, such as contention management and privatization safety.  The
resulting template is saved in the `stm_algs` or `ptm_algs` folder.  Note
that these implementations are not aware of the TM API.

To create machine code that can be used by an application, we create a file
in the `stm_instances` or `ptm_instances` folder.  These files are named by
the algorithm they use, plus the templates with which they are instantiated.
They also include all of the necessary parts from the `api` folder, so that a
single .cc file produces a single .o file that completely implements the TM
library.

## Other important folders

The `common` folder is for other code that is shared among TM implementations.
