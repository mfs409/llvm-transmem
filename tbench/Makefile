# Files to compile that don't have a main() function
CXXFILES = 

# Files to compile that do have a main() function
TARGETS = tatp tpcc tpcc_B+ bplustree bplustree_mix

# For the time being, we are going to put a layer on top of 'make all', so that
# typing 'make' will do a 'make all' that links to each of our TM
# implementations
.DEFAULT_GOAL = all_bench_all_tm

# Pull in the common makefile stuff
include makefile.common

# The default rule recursively calls make for each TM implementation.  For all
# but the first recursive call, this only links, so it's not too slow.  Note 
# that this can make a lot of executables.
all_bench_all_tm:
	TMLIBNAME=cgl_mutex $(MAKE) all
	TMLIBNAME=orec_eager_quiescence $(MAKE) all
	TMLIBNAME=orec_lazy_quiescence $(MAKE) all
	TMLIBNAME=orec_mixed_quiescence $(MAKE) all
	TMLIBNAME=norec_quiescence $(MAKE) all
	TMLIBNAME=ring_sw $(MAKE) all
	TMLIBNAME=tlrw_eager $(MAKE) all
	TMLIBNAME=pn_cgl_eager $(MAKE) all
	TMLIBNAME=pg_cgl_eager $(MAKE) all
	TMLIBNAME=pi_cgl_eager $(MAKE) all
	TMLIBNAME=pn_cgl_lazy $(MAKE) all
	TMLIBNAME=pg_cgl_lazy $(MAKE) all
	TMLIBNAME=pi_cgl_lazy $(MAKE) all
	TMLIBNAME=pn_orec_eager $(MAKE) all
	TMLIBNAME=pg_orec_eager $(MAKE) all
	TMLIBNAME=pi_orec_eager $(MAKE) all
	TMLIBNAME=pn_orec_lazy $(MAKE) all
	TMLIBNAME=pg_orec_lazy $(MAKE) all
	TMLIBNAME=pi_orec_lazy $(MAKE) all
	TMLIBNAME=pn_orec_mixed $(MAKE) all
	TMLIBNAME=pg_orec_mixed $(MAKE) all
	TMLIBNAME=pi_orec_mixed $(MAKE) all
	TMLIBNAME=pn_norec $(MAKE) all
	TMLIBNAME=pg_norec $(MAKE) all
	TMLIBNAME=pi_norec $(MAKE) all
	TMLIBNAME=pn_ring_sw $(MAKE) all
	TMLIBNAME=pg_ring_sw $(MAKE) all
	TMLIBNAME=pi_ring_sw $(MAKE) all
	TMLIBNAME=pn_tlrw_eager $(MAKE) all
	TMLIBNAME=pg_tlrw_eager $(MAKE) all
	TMLIBNAME=pi_tlrw_eager $(MAKE) all
