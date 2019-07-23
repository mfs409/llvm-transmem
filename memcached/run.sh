#!/bin/bash

if [ -f memcached_f1.txt ] ; then
    rm memcached_f1.txt
fi

#Number of threads
THREADS="1 2 4 8 16 24 32 48"
# Define a string variable with a value
TMLIB="cgl_mutex orec_eager_quiescence orec_lazy_quiescence orec_mixed_quiescence norec_quies\
cence ring_sw tlrw_eager \
pn_cgl_eager pn_orec_eager pn_orec_lazy pn_orec_mixed pn_norec pn_ring_sw pn_tlrw_eager pn_cg\
l_lazy"

PTMLIB="pg_cgl_eager pg_orec_eager pg_orec_lazy pg_orec_mixed pg_norec pg_ring_sw pg_tlrw_eag\
er pg_cgl_lazy \
pi_cgl_eager pi_orec_eager pi_orec_lazy pi_orec_mixed pi_norec pi_ring_sw pi_tlrw_eager pi_cg\
l_lazy"

for i in $THREADS
do
    for t in $TMLIB
    do
        echo "Running Memcached with $t and $i threads ..."
        numactl -N 0 ./obj64/memcached_$t -u root -p 11212 -t $i &
	numactl -N 1 /home/pantea/Project/LLVM_BENCHMARKS/llvm-tm-support/Memcached/libmemcached-1.0.18/clients/./memaslap -s localhost:11212  --concurrency=4 –threads=4  -t 1m --binary >> memcached_f1.txt
	sleep 2m
        echo -n "," >> memcached_f1.txt
	killall memcached_$t
	killall memaslap
    done
    echo -n -e "\n"  >> memcached_f1.txt
done
