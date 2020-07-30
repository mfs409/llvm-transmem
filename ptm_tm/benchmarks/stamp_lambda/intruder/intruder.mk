PROG := intruder
SRCS += decoder detector dictionary intruder packet preprocessor stream
LIBSRCS +=  lib_pair lib_queue lib_rbtree lib_thread lib_vector lib_list lib_random lib_mt19937ar
CXXFLAGS += -DMAP_USE_RBTREE