# OpenFOAM-2.4.x
insight_define_openfoam(OF24x
  FAMILY openfoam  FORK vanilla  OF_VERSION 020400
  SHORTCUT of24x  ENV_ID 240
  DIRS OpenFOAM/OpenFOAM-2.4.x OpenFOAM/OpenFOAM-2.4.0 /opt/openfoam240
  DEFINES OF23x OF24x
  INCPATH_SKIP "src/TurbulenceModels/*" # unfinished TurbulenceModels with conflicting headers
  LIBRARIES_LIKE OF23x
)
