project(testcases)

#find_package(OpenCasCade REQUIRED)

set(testcases_SOURCES 
  flatplatebl.cpp 
  decayingturbulence.cpp 
  pipe.cpp 
  channel.cpp 
  explicitvortex.cpp 
  ercoftac_squaresection180degreebend.cpp 
  airfoilsection.cpp
  freeshearflow.cpp
  )

include_directories(${toolkit_SOURCE_DIR})
include_directories(${refdata_SOURCE_DIR})
include_directories(${GSL_INCLUDE_DIR})
#include_directories(${OCC_INCLUDE_DIR})
#include_directories(${journalbearingcommon_SOURCE_DIR})
add_library(testcases SHARED ${testcases_SOURCES})
set(testcases_LIBS
  toolkit
  refdata
  ${GSL_LIBRARIES}
)
target_link_libraries(testcases ${testcases_LIBS})

install(TARGETS testcases LIBRARY DESTINATION lib)

add_pybindings(TestcaseModules "testcasemodules.i" testcases)

install_script(isCreateChannelProfiles isCreateChannelProfiles.py)