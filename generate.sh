#!/bin/bash

case "$1" in
    b)
	echo "Generating B+tree ..."
	echo "___________________________________________________________"
	cp tbench/bplustree_f1.txt result/data
	cp tbench/bplustree_f2.txt result/data
	python result/scripts/bplushtree/bpt1.py
	python result/scripts/bplushtree/bpt2.py
	;;

    c)
	echo "Generating B+tree (Mix) ..."
	echo "___________________________________________________________"
	cp tbench/bplustreemix_f1.txt result/data
	cp tbench/bplustreemix_f2.txt result/data
	python result/scripts/bplushtree_mix/bpt_mix1.py
	python result/scripts/bplushtree_mix/bpt_mix2.py
	;;

    f)
	echo "Generating TATP ..."
	echo "___________________________________________________________"
	cp tbench/tatp_f1.txt result/data
	cp tbench/tatp_f2.txt result/data
	python result/scripts/tatp/tatp1.py
	python result/scripts/tatp/tatp2.py
	;;

    e)
	echo "Generating TPCC (Hashtable) ..."
	echo "___________________________________________________________"
	cp tbench/tpcc_f1.txt result/data
	cp tbench/tpcc_f2.txt result/data
	python result/scripts/tpcc/tpcc1.py
	python result/scripts/tpcc/tpcc2.py
	;;

    d)
	echo "Generating TPCC (B+tree) ..."
	echo "___________________________________________________________"
	cp tbench/tpccbp_f1.txt result/data
	cp tbench/tpccbp_f2.txt result/data
	python result/scripts/tpcc_bp/tpcc_bp1.py
	python result/scripts/tpcc_bp/tpcc_bp2.py
	;;

    h)
	echo "Generating vacation (High) ..."
	echo "___________________________________________________________"
	cp vacation/vacation/vacationh_f1.txt  result/data
	cp vacation/vacation/vacationh_f2.txt result/data
	python result/scripts/vacation/vacation_h1.py
	python result/scripts/vacation/vacation_h2.py
	;;
    g)
	echo "Generating vacation (Low) ..."
	echo "___________________________________________________________"
	cp vacation/vacation/vacationl_f1.txt result/data
	cp vacation/vacation/vacationl_f2.txt result/data
	python result/scripts/vacation/vacation_l1.py
	python result/scripts/vacation/vacation_l2.py
	;;

    i)
	echo "Generating Memcached ..."
	echo "___________________________________________________________"
	cp memcached/memcached_f1.txt result/data
	cp memcached/memcached_f2.txt result/data
	python result/scripts/memcached/extract_f1.py
	python result/scripts/memcached/extract_f2.py
	python result/scripts/memcached/memcached_fig1.py
	python result/scripts/memcached/memcached_fig2.py
	;;

    all)
	echo "Generating B+tree ..."
        echo "___________________________________________________________"
        cp tbench/bplustree_f1.txt result/data
	cp tbench/bplustree_f2.txt result/data
        python result/scripts/bplushtree/bpt1.py
        python result/scripts/bplushtree/bpt2.py

	echo "Generating B+tree (Mix) ..."
	echo "___________________________________________________________"
	cp tbench/bplustreemix_f1.txt result/data
	cp tbench/bplustreemix_f2.txt result/data
	python result/scripts/bplushtree_mix/bpt_mix1.py
	python result/scripts/bplushtree_mix/bpt_mix2.py

	echo "Generating TATP ..."
	echo "___________________________________________________________"
	cp tbench/tatp_f1.txt result/data
	cp tbench/tatp_f2.txt result/data
	python result/scripts/tatp/tatp1.py
	python result/scripts/tatp/tatp2.py

	echo "Generating TPCC (Hashtable) ..."
	echo "___________________________________________________________"
	cp tbench/tpcc_f1.txt result/data
	cp tbench/tpcc_f2.txt result/data
	python result/scripts/tpcc/tpcc1.py
	python result/scripts/tpcc/tpcc2.py
	
	echo "Generating TPCC (B+tree) ..."
	echo "___________________________________________________________"
	cp tbench/tpccbp_f1.txt result/data
	cp tbench/tpccbp_f2.txt result/data
	python result/scripts/tpcc_bp/tpcc_bp1.py
	python result/scripts/tpcc_bp/tpcc_bp2.py

	echo "Generating vacation (High) ..."
	echo "___________________________________________________________"
	cp vacation/vacation/vacationh_f1.txt  result/data
	cp vacation/vacation/vacationh_f2.txt result/data
	python result/scripts/vacation/vacation_h1.py
	python result/scripts/vacation/vacation_h2.py

	echo "Generating vacation (Low) ..."
	echo "___________________________________________________________"
	cp vacation/vacation/vacationl_f1.txt result/data
	cp vacation/vacation/vacationl_f2.txt result/data
	python result/scripts/vacation/vacation_l1.py
	python result/scripts/vacation/vacation_l2.py

	echo "Generating Memcached ..."
	echo "___________________________________________________________"
	cp memcached/memcached_f1.txt result/data
	cp memcached/memcached_f2.txt result/data
	python result/scripts/memcached/extract_f1.py
	python result/scripts/memcached/extract_f2.py
	python result/scripts/memcached/memcached_fig1.py
	python result/scripts/memcached/memcached_fig2.py
	;;
esac
