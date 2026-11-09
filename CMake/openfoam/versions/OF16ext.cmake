# OpenFOAM-1.6-ext
insight_define_openfoam(OF16ext
  FAMILY extend  FORK extend  OF_VERSION 010600
  SHORTCUT of16ext  ENV_ID 160
  DIRS OpenFOAM/OpenFOAM-1.6-ext
  LIBRARIES AUTO
  NO_DEPLIBS
  LEGACY_ENV_SCRIPT
  EXTRA_LINK_LIBS
    @FOAM_MPI_LIBBIN@/libPstream.so
    @PARMETIS_LIB_DIR@/libparmetis.so
    @SCOTCH_LIB_DIR@/libscotch.so
    @MESQUITE_LIB_DIR@/libmesquite.so
)
