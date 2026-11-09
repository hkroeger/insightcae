# OpenFOAM-2.2 Engys edition
insight_define_openfoam(OF22eng
  FAMILY openfoam  FORK engys  OF_VERSION 020200
  SHORTCUT of22eng  ENV_ID 220
  DIRS OpenFOAM/OpenFOAM-2.2_engysEdition OpenFOAM/OpenFOAM-2.2_engysEdition-beta
  LIBRARIES AUTO
  NO_DEPLIBS
  EXTRA_LINK_LIBS @LIB_DIR@/@FOAM_MPI@/libPstream.so
)
