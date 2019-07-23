#!/bin/bash

case "$1" in
    b)
	cd tbench
	echo "Running B+tree ..."
	echo "___________________________________________________________"
	./run_bplustree.sh
	cd ../
    ;;

    c)
	cd tbench
	echo "Running B+tree (Mix) ..."
	echo "___________________________________________________________"
	./run_bplustreemix.sh
	cd ..
    ;;

    f)
	cd tbench   
	echo "Running TATP ..."
	echo "___________________________________________________________"
	./run_tatp.sh
	cd ..
    ;;


    e)
	cd tbench
	echo "Running TPCC (Hashtable) ..."
	echo "___________________________________________________________"
	./run_tpcc.sh
	cd ..
    ;;

    d)
	cd tbench
	echo "Running TPCC (B+tree) ..."
	echo "___________________________________________________________"
	./run_tpcc_B.sh
	cd ..
    ;;


    h)
	cd vacation/vacation
	echo "Running vacation (High) ..."
	echo "___________________________________________________________"
	./run_high.sh
	./run_high_ptm.sh
	cd ../..
    ;;

    g)
	cd vacation/vacation
	echo "Running vacation (Low) ..."
	echo "___________________________________________________________"
	./run_low.sh
	./run_low_ptm.sh
	cd ../..
    ;;
    

    i)
	cd memcached
	echo "Running Memcached ..."
	echo "___________________________________________________________"
	./run.sh
	./run_ptm.sh
	cd ..
	;;

    all)
	cd tbench
	echo "Running B+tree ..."
	echo "___________________________________________________________"
	./run_bplustree.sh
	
	echo "Running B+tree (Mix) ..."
	echo "___________________________________________________________"
	run_bplustreemix.sh

	echo "Running TATP ..."
	echo "___________________________________________________________"
	./run_tatp.sh
	
	echo "Running TPCC (Hashtable) ..."
	echo "___________________________________________________________"
	./run_tpcc.sh
	
	echo "Running TPCC (B+tree) ..."
	echo "___________________________________________________________"
	./run_tpcc_B.sh
	
	cd ..
	cd vacation/vacation
	
	echo "Running vacation (High) ..."
	echo "___________________________________________________________"
	./run_high.sh
	./run_high_ptm.sh
	
	echo "Running vacation (Low) ..."
	echo "___________________________________________________________"
	./run_low.sh
	./run_low_ptm.sh
	
	cd ../../
	cd memcached
	
	echo "Running Memcached ..."
	echo "___________________________________________________________"
	./run.sh
	./run_ptm.sh
	
	cd..
	;;
esac
