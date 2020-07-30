all:
	API=lambda BENCH_ROOT=../ $(MAKE) -f ../memcached.mk
	API=lambda_lite BENCH_ROOT=../ $(MAKE) -f ../memcached.mk

clean:
	API=lambda BENCH_ROOT=../ $(MAKE) -f ../memcached.mk clean
	API=lambda_lite BENCH_ROOT=../ $(MAKE) -f ../memcached.mk clean