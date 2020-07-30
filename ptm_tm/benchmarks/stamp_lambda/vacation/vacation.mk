PROG := vacation
SRCS += client customer manager reservation vacation
LIBSRCS += lib_list lib_pair lib_rbtree lib_thread lib_random lib_mt19937ar
CXXFLAGS += -DLIST_NO_DUPLICATES
CXXFLAGS += -DMAP_USE_RBTREE