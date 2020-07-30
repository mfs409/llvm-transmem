PROG := ssca2
SRCS += alg_radix_smp computeGraph createPartition cutClusters findSubGraphs \
        genScalData getStartLists getUserParameters globals ssca2
LIBSRCS += lib_thread
#CXXFLAGS += -DUSE_PARALLEL_DATA_GENERATION
#CXXFLAGS += -DWRITE_RESULT_FILES
CXXFLAGS += -DENABLE_KERNEL1
#CXXFLAGS += -DENABLE_KERNEL2 -DENABLE_KERNEL3
#CXXFLAGS += -DENABLE_KERNEL4
LDFLAGS += -lm