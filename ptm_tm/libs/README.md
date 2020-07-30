# TM and PTM Libraries

The `llvm-transmem/libs` folder is the home for all of the TM and PTM algorithms
that are available for use in our system.  Note that these algorithms include
traditional STM algorithms, Hybrid algorithms that try to use hardware TM
support when it is available, and algorithms for performing transactions over
persistent memory.

## Structure of a TM Implementation

In order to share code as much as possible, and to support as many algorithms
as possible without creating too much clutter, we use the following strategy.
We begin with an algorithm category, typically based on fundamental
synchronization decisions, such as the type of metadata used to mediate
concurrent access to a location and how updates to the shared memory are
managed.  We parameterize this implementation based on implementation
options, such as contention management and privatization safety.  The
resulting template is saved in the `stm_algs` or `ptm_algs` folder.  Note
that these implementations are not aware of the TM API.

To create machine code that can be used by an application, we create a file in
the `stm_instances` or `ptm_instances` folder.  These files are named by the
algorithm they use, plus the templates with which they are instantiated. They
also include all of the necessary parts from the `api` folder, so that a single
.cc file produces a single .o file that completely implements the TM library.
These `api` parts are generated via macros.

## Other important folders

The `common` folder is for other code that is shared among TM implementations.

The `../common` folder is for headers that are required by more than one
component of our system (e.g., libraries, plugin, benchmarks).

## Algorithms

Below, we briefly describe the algorithms supported in this folder:

* Coarse Lock (`cgl.h`) protects all transactions with a single coarse-grained
  lock.  It is a baseline, but it doesn't scale.  
* TL2 (`tl2.h`) uses commit-time locking and ownership records.  It does not
  have timestamp extension.  See Dice-DISC-2006.
* TinySTM:WriteThrough (`orec_eager.h`) uses encounter-time locking, undo
  logging, and ownership records.  It does have timestamp extension.  See
  Felber-PPoPP-2008.  This is also the default STM in GCC.
* TinySTM:WriteBack (`orec_mixed.h`) uses encounter-time locking, redo logging,
  and ownership records.  It does have timestamp extension.  See
  Felber-PPoPP-2008.
* TinySTM:CTL (`orec_lazy.h`) uses commit-time locking and ownership records.
  It does have timestamp extension.  See Spear-PPoPP-2009.
* RingSTM:SingleWriter (`ring_sw.h`) uses a fixed-size ring of bit vectors to
  publish transaction write sets.  See Spear-SPAA-2008.
* RingSTM:MultiWriter (`ring_mw.h`) is an optimized version of
  RingSTM:SingleWriter that allows concurrent nonconflicting writeback without
  compromising privatization safety.  See Spear-SPAA-2008.
* TLRW (`tlrw_eager.h`) uses encounter-time locking and read locking (via byte
  locks) to avoid all validation overhead.  See Dice-SPAA-2010.
* TML:Eager (`tml_eager.h`) uses a single ownership record and encounter-time
  locking to achieve low latency and scalability for concurrent read-only
  transactions. See Spear-EuroPar-2010, or the Sun Microsystems "M4" patent.
* TML:Lazy (`tml_lazy.h`) is a version of TML with commit-time locking and redo
  logging.
* NOrec (`norec.h`) uses value-based validation and commit-time locking.  See
  Dalessandro-PPoPP-2010.
* Cohorts (`cohorts.h`) bundles concurrent transactions into groups that commit
  together, thereby avoiding in-flight validation and memory fences on ARM.  See
  Ruan-CGO-2013.
* HTM:GL (`htm_gl.h`) uses HTM resources to execute transactions, and falls back
  to a single lock otherwise.  See Yoo-SC-2013.
* HybridNOrec:TwoCounter (`hybrid_norec_two_counter.h`) is a version of the
  NOrec algorithm that exploits HTM resources.  This version does not require
  hardware support for non-transactional reads or writes.  See
  Dalessandro-ASPLOS-2011.
* Reduced Hardware NOrec (`reduced_hardware_norec.h`) uses the "reduced
  hardware" technique to accelerate NOrec.  See Matveev-ASPLOS-2015.
* Hybrid RingSTM (`hybrid_ring_sw.h`) is an unpublished version of RingSTM that
  adds HTM acceleration.

Note that there are a variety of interesting instantiations of these algorithms.
Two points deserve special attention:

* Variations between "safe" and "unsafe" are intended to approximate the
  overhead of using `atomic_ref`, when it becomes available.
* The `rdtscp_lazy` instantiation of `orec_lazy.h` replaces the global counter
  with a hardware counter, as in Ruan-TACO-2013.  This has noticeable impact,
  especially on NUMA x86 systems.

## Persistence Notes

Many of the above algorithms have persistent versions in the `ptm` folders.  The
algorithmic changes primarily deal with data structures (e.g., making redo and
undo logs persistent) and adding fence and flush commands.  In the instantiation
folder, there are three classes: `pn`, `pg`, and `pi`.  The `pn` instances
represent naïve transformations of STM algorithms to PTM.  The `pg` instances
add dynamic optimizations that are general to any persistence model.  The `pi`
instances leverage optimizations for an idealized setting, with static
separation and guaranteed alignment of all NVM objects.  For more details, see
Zardoshti-PACT-2019.
