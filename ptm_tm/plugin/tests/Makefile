# Each test lives in a single file.  That file uses #defines to describe
# three parts: TEST_DRIVER, TEST_OFILE1, and TEST_OFILE2.  This Makefile will
# use the #defines to produce *three* .o files from a test file.  The DRIVER
# is just a main function that runs the test and reports it.  The code of the
# test is split between the other two .o files.  In this way, we can have
# tests that do complex things across translation units (TUs), avoid
# inlining, etc., but we can still see the whole test at once.
#
# This Makefile names the .o files .1.o, .2.o, and .x.o, and produces an
# executable with an .exe extension.  We use the TM plugin when building the
# .1.o and .2.o files.
#
# The "build" target simply builds all .o and .exe files.  The "test" target
# also runs all of the .exe files.

# The names of the unit tests.  Each should be a .cc file in the units/
# folder
UNITS = clone_001 clone_002 clone_003 \
        load_001 load_002 load_003 load_004 load_005 load_006 load_007 load_008 load_009 \
        store_001 store_002 store_003 store_004 store_005 store_006 store_007 store_008 store_009 \
        ldst_001 ldst_002 ldst_003 ldst_004 ldst_005 ldst_006 ldst_007 ldst_008 ldst_009 \
        callinst_001 callinst_002 callinst_003 callinst_004 callinst_005 callinst_006 callinst_007 callinst_008 callinst_009 \
        aload_001 aload_002 aload_003 aload_004 aload_005 aload_006 aload_007 aload_008  \
        astore_001 astore_002 astore_003 astore_004 astore_005 astore_006 astore_007 astore_008  \
        arload_001 arload_002 arload_003 arload_004 arload_005 arload_006 arload_007 arload_008  \
        arstore_001 arstore_002 arstore_003 arstore_004 arstore_005 arstore_006 arstore_007 arstore_008 \
        vload_001 vload_002 vload_003 vload_004 vload_005 vload_006 vload_007 vload_008 vload_009 \
        vstore_001 vstore_002 vstore_003 vstore_004 vstore_005 vstore_006 vstore_007 vstore_008 vstore_009 \
        capi_001 capi_002 capi_003 capi_004 capi_005 capi_006 capi_007 capi_008 capi_009 capi_010 capi_011 capi_012 capi_013 \
        dispatch_001 dispatch_002 dispatch_003 dispatch_004 dispatch_005 dispatch_006 dispatch_007 dispatch_008 \
        intrin_001 intrin_002 intrin_003 \
        alloc_001 alloc_002 alloc_003 alloc_004 alloc_005 \
        lambda_001 lambda_002 lambda_003 lambda_004 lambda_005 lambda_006 lambda_007 \
        invokeinst_001 invokeinst_002 invokeinst_003 invokeinst_004 invokeinst_005 \
        asm_001 \
        selfmod_001 \
        aligned_001 \
        maskvec_001 maskvec_002 \
        ssevec_001 ssevec_002 \
        ivsv_001 \
        thrwctch_001 thrwctch_002 thrwctch_003 thrwctch_004 thrwctch_005 thrwctch_006 thrwctch_007 thrwctch_008 thrwctch_009 thrwctch_010 thrwctch_011 thrwctch_012 \
        unsafeopt_001 unsafeopt_002 unsafeopt_003 unsafeopt_004 \
        cfg_001 cfg_002 cfg_003 cfg_004 cfg_005 cfg_006 cfg_007 cfg_008 cfg_009 cfg_010

# NB: We do not run thrwctch_013, because it is not implemented yet.

# The name of the special TM library that we build/use for unit testing.  It
# should be in the /libs folder.
LIBS = countingtm

# Generated Files
LIBOFILES  = $(patsubst %, $(ODIR)/%.o, $(LIBS)) 
UNITOFILES = $(patsubst %, $(ODIR)/%.1.o $(ODIR)/%.2.o $(ODIR)/%.x.o, $(UNITS))
EXEFILES   = $(patsubst %, $(ODIR)/%.exe, $(UNITS))
DEPS       = $(patsubst %.o, %.d, $(UNITOFILES) $(LIBOFILES)))

# A generated list of targets; used to run the tests from the makefile
TEST_CMDS = $(patsubst %, %.test, $(UNITS))

# We are only concerned with 64-bit code
BITS = 64

# Directory names
ODIR          := ./obj$(BITS)
output_folder := $(shell mkdir -p $(ODIR))

# Configure the compiler
CXX      = clang++-10
CXXFLAGS = -MMD -O3 -m$(BITS) -std=c++11 -Wall -Werror -fPIC #-emit-llvm
LD       = g++
LDFLAGS  = -m$(BITS)

# Configure the API
CXXFLAGS += -DTM_USE_LAMBDA -DTM_RAII_OVERRIDE

# The name of the compiler pass
CXXPASSSOFILE  = ../plugin/build/libtmplugin.so

# Set up the flags for the compiler plugin, so we can invoke it from clang++
PLUGINFLAGS = -Xclang -load -Xclang $(CXXPASSSOFILE)

# Flags for the test harness
DOT1 = -DTEST_OFILE1
DOT2 = -DTEST_OFILE2
DOTX = -DTEST_DRIVER

# Standard makefile targets
.DEFAULT_GOAL = all
.PRECIOUS: $(LIBOFILES) $(UNITOFILES) $(EXEFILES)
.PHONY: all build test clean

# Build all tests, and run them
all: test

# Just build stuff, don't run it
build: $(EXEFILES)

# Execute the tests (rebuild them if necessary)
test: $(TEST_CMDS)

# Rules to build the three types of unit .o files, the library .o file, and
# the executables
$(ODIR)/%.x.o: units/%.cc $(CXXPASSSOFILE)
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS) $(DOTX)
$(ODIR)/%.1.o: units/%.cc $(CXXPASSSOFILE)
	@echo "[CXX] $< --> $@"
	@$(CXX) $(PLUGINFLAGS) $< -o $@ -c $(CXXFLAGS) $(DOT1)
$(ODIR)/%.2.o: units/%.cc $(CXXPASSSOFILE)
	@echo "[CXX] $< --> $@"
	@$(CXX) $(PLUGINFLAGS) $< -o $@ -c $(CXXFLAGS) $(DOT2)
$(ODIR)/%.o: libs/%.cc $(CXXPASSSOFILE)
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)
$(ODIR)/%.exe: $(ODIR)/%.1.o $(ODIR)/%.2.o $(ODIR)/%.x.o $(LIBOFILES)
	@echo "[LD] $^ --> $@"
	@$(LD) $(LDFLAGS) $^ -o $@ 

# Rules for running the tests
%.test: $(ODIR)/%.exe
	@$<

# clean by clobbering the build folder
clean:
	@echo Cleaning up...
	@rm -rf $(ODIR)

# Include dependencies
-include $(DEPS)
