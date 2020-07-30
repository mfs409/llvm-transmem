all:
	API=raii BENCH_ROOT=../ $(MAKE) -f ../memcached.mk
	API=raii_lite BENCH_ROOT=../ $(MAKE) -f ../memcached.mk

clean:
	API=raii BENCH_ROOT=../ $(MAKE) -f ../memcached.mk clean
	API=raii_lite BENCH_ROOT=../ $(MAKE) -f ../memcached.mk clean