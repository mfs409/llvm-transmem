PROG := labyrinth
SRCS += coordinate grid labyrinth maze router
LIBSRCS += lib_list lib_pair lib_queue lib_thread lib_vector
CXXFLAGS += -DUSE_EARLY_RELEASE
LDFLAGS += -lm