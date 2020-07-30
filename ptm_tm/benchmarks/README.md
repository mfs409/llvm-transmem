# Benchmarks

The `llvm-transmem/benchmarks` folder has several benchmarks.  These demonstrate
how to use our infrastructure, and also provide an opportunity to evaluate
performance.

We provide the following benchmarks:

* Microbenchmarks (`ubench`) are data structure stress tests.
* STAMP (`stamp`) benchmarks are traditional "small application" benchmarks from
  Minh-IISWC-2008.
* TBench (`tbench`) is a set of persistence-focused benchmarks from
  Liu-ASPLOS-2017.
* Memcached (`memcached`) is a transactional version of the memcached in-memory
  key/value store.

## Notes

On account of persistence support, some of these benchmarks have been modified
to use aligned allocation.

Each benchmark will be built four times.  Two of the builds are for the RAII and
Lambda APIs with full support for STM.  The other two correspond to the RAII and
Lambda APIs when the run-time system only supports un-instrumented transactions
(basic HTM or a single lock).  These latter versions have the `_lite` suffix.

Note that each benchmark will be linked against *every* TM algorithm.  This can
take a long time, and produce a very large build folder.  Passing the `-j` flag
to `make` is recommended.