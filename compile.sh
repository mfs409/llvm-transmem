echo "Compiling the plugin and libraries ..."
echo "___________________________________________________________"
make clean
make

echo "Compiling B+tree, TPCC and TATP .."
echo "___________________________________________________________"
cd tbench
make clean
make
cd ..

echo "Compiling Vacation ..."
echo "___________________________________________________________"
cd vacation/vacation
make clean
make
cd ../../

echo "Compiling Memcached ..."
echo "___________________________________________________________"
cd memcached
rm -rf obj64
./build.sh
cd ..
