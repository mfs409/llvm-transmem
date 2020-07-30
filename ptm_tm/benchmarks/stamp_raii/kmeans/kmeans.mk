PROG := kmeans
SRCS += cluster common kmeans normal
LIBSRCS += lib_thread
LDFLAGS += -lm