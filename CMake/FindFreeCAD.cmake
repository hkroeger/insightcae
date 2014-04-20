# Try to find FreeCAD installation
# Once done this will define
#
# FC_FOUND          - system has FreeCAD
# FC_MOD_DIR        - where the OCC include directory can be found



FIND_PATH(FC_MOD_DIR NAMES PartDesign/__init__.py
  HINTS
  /usr/Mod
)

message(STATUS ${FC_MOD_DIR})


SET(FC_FOUND FALSE)

IF(OF_LIBRARY)
  #GET_FILENAME_COMPONENT(OF_LIB_DIR ${OF_LIBRARY} PATH)
  #message(STATUS ${OF_LIB_DIR})
  SET(FC_FOUND TRUE)
  #SET(OF_CXX_FLAGS "-Dlinux64 -DWM_DP -O3 -DNoRepository -ftemplate-depth-40 -fPIC -fpermissive")
  #message(STATUS ${OF_CXX_FLAGS})
  #SET(OF_SHARED_LINKER_FLAGS "-shared -Xlinker --add-needed -Wl,--no-as-needed")
  #SET(OF_LINK_FLAGS "-Xlinker --add-needed -Wl,--no-as-needed")  
ENDIF(OF_LIBRARY)
