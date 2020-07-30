# Turn on the LAMBDA API, set up flags, select libraries
API           = lambda
CCPASSSOFILE  = $(TM_ROOT)/plugin/plugin/build/libtmplugin.so
CXXFLAGS     += -Xclang -load -Xclang $(CCPASSSOFILE)
API_LIBS      = $(TM_LIB_NAMES)
