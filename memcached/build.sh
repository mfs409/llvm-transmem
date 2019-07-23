FILE=obj64
rm -rf $FILE
mkdir $FILE

echo "Compiling Memcached"
# Define a string variable with a value
FILEVAL="slabs stats memcached items hash assoc daemon cache thread util"

for f in $FILEVAL
do
    clang++-6.0  -Xclang -load -Xclang ../plugin/build/libtmplugin.so -o obj64/memcached-$f.o -c -DHAVE_CONFIG_H  -DNDEBUG -O3 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -MMD -m64 -std=c++11  $f.cc
done

TMLIB="cgl_mutex orec_eager_quiescence orec_lazy_quiescence orec_mixed_quiescence norec_quiescence ring_sw tlrw_eager \
pn_cgl_eager pn_orec_eager pn_orec_lazy pn_orec_mixed pn_norec pn_ring_sw pn_tlrw_eager pn_cgl_lazy \
pg_cgl_eager pg_orec_eager pg_orec_lazy pg_orec_mixed pg_norec pg_ring_sw pg_tlrw_eager pg_cgl_lazy \
pi_cgl_eager pi_orec_eager pi_orec_lazy pi_orec_mixed pi_norec pi_ring_sw pi_tlrw_eager pi_cgl_lazy"

for t in $TMLIB
do 	 
    g++ -lm -m64 -pthread  -O3 -Wall -Werror -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o obj64/memcached_$t obj64/memcached-memcached.o obj64/memcached-hash.o obj64/memcached-slabs.o obj64/memcached-items.o obj64/memcached-assoc.o obj64/memcached-thread.o obj64/memcached-daemon.o obj64/memcached-stats.o obj64/memcached-util.o obj64/memcached-cache.o  ../libs/./obj64/$t.o  -static -levent
done

