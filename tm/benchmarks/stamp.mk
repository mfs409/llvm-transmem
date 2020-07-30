# Get the names of all TMs
TM_ROOT = $(BENCH_ROOT)/../
include $(TM_ROOT)/libs/tm_names.mk

# Get the specific configuration for this benchmark
include $(BENCH_ROOT)/$(STAMP_FOLDER)/$(PROG)/$(PROG).mk

# Get configuration info specific to the API that is being built right now.
# This will break if stamp.mk is used directly, because API needs to be
# defined. That's intentional.
include $(TM_ROOT)/common/$(API).mk

# Compiler configuration
BITS     ?= 64
CXX       = clang++-10
CXXFLAGS += -MMD -O3 -m$(BITS) -std=c++11 -Wall -I./lib/ -I./ -fPIC -march=native
LD        = g++
LDFLAGS  += -m$(BITS) -pthread -static

# The name of the folder where everything will go
ODIR=$(PROG)/obj$(BITS)/$(API)
output_folder := $(shell mkdir -p $(ODIR))

# Set up all the things we build
EXEFILES       = $(foreach t, $(API_LIBS), $(ODIR)/$(PROG).$t.exe)
OFILES         = $(patsubst %, $(ODIR)/%.o, $(SRCS) $(LIBSRCS))
DEPS           = $(patsubst %.o, %.d, $(OFILES))

# Prevent .o files from getting cleaned up
.PRECIOUS: $(OFILES)

# target 'all' will build the .o files, then recurse to link the .exe files
all: $(EXEFILES)

# Build an object file from a source file
$(ODIR)/%.o: $(PROG)/%.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)
$(ODIR)/lib_%.o: lib/%.cc
	@$(CXX) $< -o $@ -c $(CXXFLAGS)

# Link an executable... we don't have good dependencies on this, but names are
# hard enough to forge that we can expect this to only be called recursively, in
# which case the Makefile will get everything set up correctly.
$(ODIR)/$(PROG).%.exe: $(OFILES)
	@echo "[LD] $@"
	@$(LD) $(TM_ROOT)/libs/obj$(BITS)/$(basename $(subst $(ODIR)/$(PROG)., , $@)).o $(OFILES) -o $@ $(LDFLAGS)

# clean by clobbering the build folder
clean:
	rm -rf $(ODIR)

# Dependencies
-include $(DEPS)
