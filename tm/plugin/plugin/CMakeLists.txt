#
# Standard configuration when building anything related to LLVM
#
cmake_minimum_required(VERSION 3.1)
project(llvm_transmem)
find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

#
# We will build two libraries: libtmplugin.so and libtmplugin_wo.so.  Each is a
# plugin that we run when compiling, in order to achieve transactional
# instrumentation.  They are built from the same code base, but with different
# #define-based customizations
#

#
# Files associated with libtmplugin.so... this is the default version of the 
# plugin
#
add_library(tmplugin MODULE
    tm_plugin.cc
    signatures.cc
    mappings.cc
    discovery.cc
    function_transform.cc
    boundary_transform.cc
    raii.cc
    optimizations.cc
)

#
# Files associated with libtmplugin_wo.so... this is the version of the plugin 
# that only instruments writes
#
add_library(tmplugin_wo MODULE
    tm_plugin.cc
    signatures.cc
    mappings.cc
    discovery.cc
    function_transform.cc
    boundary_transform.cc
    raii.cc
    optimizations.cc
)

#
# Set compiler features that are the same for both libraries
#
set_property(TARGET tmplugin PROPERTY CXX_STANDARD 17)
set_property(TARGET tmplugin_wo PROPERTY CXX_STANDARD 17)
target_compile_features(tmplugin PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(tmplugin_wo PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(tmplugin PROPERTIES COMPILE_FLAGS "-fno-rtti")
set_target_properties(tmplugin_wo PROPERTIES COMPILE_FLAGS "-fno-rtti")

#
# Set up compiler flags that let us build two libraries with different behaviors
#
set_target_properties(tmplugin PROPERTIES COMPILE_FLAGS "-DINST_READ=true")
set_target_properties(tmplugin_wo PROPERTIES COMPILE_FLAGS "-DINST_READ=false")

#
# per https://github.com/sampsyo/llvm-pass-skeleton/issues/7, we need to
# link these .so files specially since we are still using the legacy pass 
# manager
#
set_target_properties(tmplugin PROPERTIES LINK_FLAGS "-Wl,-znodelete")
set_target_properties(tmplugin_wo PROPERTIES LINK_FLAGS "-Wl,-znodelete")

#
# OS X-specific configuration.  Not tested, as we always work in Docker, but 
# still good to have for reference.
#
if(APPLE)
    set_target_properties(tmplugin PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    set_target_properties(tmplugin_wo PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
