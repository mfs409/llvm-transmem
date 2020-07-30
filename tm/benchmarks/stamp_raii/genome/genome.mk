PROG := genome
SRCS += gene genome segments sequencer table
LIBSRCS += lib_bitmap lib_hash lib_hashtable lib_pair lib_list lib_thread \
           lib_vector lib_random lib_mt19937ar 
CXXFLAGS += -DLIST_NO_DUPLICATES
CXXFLAGS += -DCHUNK_STEP1=12