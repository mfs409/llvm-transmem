# Each TM library will exist entirely in a single file.  The current list of STM
# and PTM implementations is in a separate file
include ../Makefile.libnames
LIBS = $(TM_LIB_NAMES)

# This makefile will create a .o for each of the TM libraries
OFILES    = $(patsubst %, $(ODIR)/%.o, $(LIBS))

# Dependencies that will be generated
DEPS      = $(patsubst %, $(ODIR)/%.d, $(LIBS))

# Let the programmer choose 32 or 64 bits, but default to 32
BITS ?= 64

# Directory names
ODIR := ./obj$(BITS)
output_folder := $(shell mkdir -p $(ODIR))

# Configure the compiler
CXX = clang++-6.0
CXXFLAGS = -mrtm -MMD -O3 -m$(BITS) -ggdb -std=c++11 -Wall -Werror -I../include -fPIC # -emit-llvm -flto
LDFLAGS = -m$(BITS)

# Best to be safe...
.DEFAULT_GOAL = all
.PRECIOUS: $(OFILES)
.PHONY: all clean

# Goal is to build all libraries as .o files
all: $(OFILES)

# Rule for building object files [legacy implementations]
$(ODIR)/%.o: legacy/%.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)

# Rules for building object files [modern (templated) implementations]
$(ODIR)/%.o: stm_instances/%.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)
$(ODIR)/%.o: ptm_instances/%.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)

# clean by clobbering the build folder
clean:
	@echo Cleaning up...
	@rm -rf $(ODIR)

# Include dependencies
-include $(DEPS)
