#
# Standard configuration when building anything related to LLVM
#
cmake_minimum_required(VERSION 3.1)
find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

#
# We will build one library: libtmplugin.so.  It corresponds to a plugin that we 
# run when compiling.
#

#
# Files associated with libtmplugin.so
#
add_library(tmplugin MODULE 
    tm_plugin.cc
    signatures.cc
    discovery.cc
    function_transform.cc
    boundary_transform.cc
    mappings.cc
    optimizations.cc
)

#
# Turn on C++11, turn off C++ RTTI.
#
target_compile_features(tmplugin PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(tmplugin PROPERTIES COMPILE_FLAGS "-fno-rtti")

#
# OS X-specific configuration
#
if(APPLE)
    set_target_properties(tmplugin PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)