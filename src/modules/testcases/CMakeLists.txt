project(testcases)

#find_package(OpenCasCade REQUIRED)

set(testcases_SOURCES pipe.cpp explicitvortex.cpp) # channel.cpp)

include_directories(${toolkit_SOURCE_DIR})
#include_directories(${OCC_INCLUDE_DIR})
#include_directories(${journalbearingcommon_SOURCE_DIR})
add_library(testcases SHARED ${testcases_SOURCES})
target_link_libraries(testcases 
toolkit
)

install(TARGETS testcases LIBRARY DESTINATION lib)

