# Each TM library will exist entirely in a single file.  The current list of STM
# and PTM implementations is in a separate file
include tm_names.mk
LIBS = $(TM_LIB_NAMES)

# This makefile will create a .o for each of the TM libraries
OFILES = $(patsubst %, $(ODIR)/%.o, $(LIBS))

# Dependencies that will be generated
DEPS = $(patsubst %, $(ODIR)/%.d, $(LIBS))

# Let the programmer choose 32 or 64 bits, but default to 32
BITS ?= 64

# Directory names
ODIR          := ./obj$(BITS)
output_folder := $(shell mkdir -p $(ODIR))

# Configure the compiler
CXX      = clang++-10
CXXFLAGS = -mrtm -MMD -O3 -m$(BITS) -ggdb -std=c++17 -Wall -Werror -fPIC \
           -march=native -Wextra

# Configure Makefile targets
.DEFAULT_GOAL = all
.PRECIOUS: $(OFILES)
.PHONY: all clean

# build all libraries as .o files
all: $(OFILES)

# clean by clobbering the build folder
clean:
	@echo Cleaning up...
	@rm -rf $(ODIR)

# build rules
$(ODIR)/%.o: stm_instances/%.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)

$(ODIR)/%.o: ptm_instances/%.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)

# Include dependencies
-include $(DEPS)
