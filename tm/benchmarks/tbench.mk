# Get the names of all TMs
TM_ROOT = $(BENCH_ROOT)/../
include $(TM_ROOT)/libs/tm_names.mk

# Names of benchmark files / names of benchmarks.  We never split a tbench
# microbenchmark across multiple .cc files, so we can expect that each of these
# files will have a main() function, and that we will have one .cc file per
# benchmark
TBENCH_NAMES = tatp tpcc tpcc_B+ bplustree

# Get configuration info specific to the API that is being built right now. This
# will break if tbench.mk is used directly, because API needs to be defined.
# That's intentional.
include $(TM_ROOT)/common/$(API).mk

# Compiler configuration
BITS     ?= 64
CXX       = clang++-10
CXXFLAGS += -MMD -O3 -m$(BITS) -std=c++17 -Wall -Werror -fPIC -march=native
LD        = g++
LDFLAGS  += -m$(BITS) -pthread

# The name of the folder where everything will go
ODIR=obj$(BITS)/$(API)
output_folder := $(shell mkdir -p $(ODIR))

# Set up all the things we build, so that our recursions work correctly
EXEFILES       = $(foreach t, $(API_LIBS), $(ODIR)/$(TBENCH).$t.exe)
OFILES         = $(patsubst %, $(ODIR)/%.o, $(TBENCH_NAMES))
DEPS           = $(patsubst %.o, %.d, $(OFILES))
TBENCH_TARGETS = $(patsubst %, %.tbench, $(TBENCH_NAMES))

# Prevent .o files from getting cleaned up
.PRECIOUS: $(OFILES)

# target 'all' will build the .o files, then recurse to link the .exe files
all: $(TBENCH_TARGETS)
%.tbench: $(ODIR)/%.o
	@TBENCH=$(basename $@) make linkstep -f $(BENCH_ROOT)/tbench.mk
linkstep: $(EXEFILES)

# Build an object file from a source file
$(ODIR)/%.o: %.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)

# Link an executable... we don't have good dependencies on this, but names are
# hard enough to forge that we can expect this to only be called recursively, in
# which case the Makefile will get everything set up correctly.
$(ODIR)/$(TBENCH).%.exe:
	@echo "[LD] $@"
	@$(LD) $(TM_ROOT)/libs/obj$(BITS)/$(basename $(subst $(ODIR)/$(TBENCH)., , $@)).o $(ODIR)/$(TBENCH).o -o $@ $(LDFLAGS)

# clean by clobbering the build folder
clean:
	rm -rf $(ODIR)

# Dependencies
-include $(DEPS)
