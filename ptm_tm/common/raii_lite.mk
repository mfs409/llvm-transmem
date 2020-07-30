# Turn on the RAII_LITE API, set up flags, select libraries
API       = raii_lite
CXXFLAGS += -DTM_USE_RAII_LITE
API_LIBS  = htm_gl cgl_mutex
