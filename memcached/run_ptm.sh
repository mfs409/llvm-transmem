#!/bin/bash

if [ -f memcached_f2.txt ] ; then
    rm memcached_f2.txt
fi

#Number of threads
THREADS="1 2 4 8 16 24 32 48"
# Define a string variable with a value

PTMLIB="pg_cgl_eager pg_orec_eager pg_orec_lazy pg_orec_mixed pg_norec pg_ring_sw pg_tlrw_eager pg_cgl_lazy \
pi_cgl_eager pi_orec_eager pi_orec_lazy pi_orec_mixed pi_norec pi_ring_sw pi_tlrw_eager pi_cgl_lazy"

for i in $THREADS
do
    for t in $PTMLIB
    do
        echo "Running Memcached with $t and $i threads ..."
        numactl -N 1 ./obj64/memcached_$t -u root -p 11214 -t $i &
	numactl -N 0 /home/pantea/Project/LLVM_BENCHMARKS/llvm-tm-support/Memcached/libmemcached-1.0.18/clients/./memaslap -s localhost:11214  --concurrency=4 –threads=4  -t 1m --binary >> memcached_f2.txt
	sleep 2m
        echo -n "," >> memcached_f2.txt
	killall memcached_$t
	killall memaslap
    done
    echo -n -e "\n"  >> memcached_f2.txt
done
