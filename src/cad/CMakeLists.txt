project(insightcad)

find_package(OpenCasCade REQUIRED)

set(insightcad_SOURCES solidmodel.cpp )

include_directories(${toolkit_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OCC_INCLUDE_DIR})
link_directories(${OCC_LIBRARY_DIR})

add_library(insightcad SHARED ${insightcad_SOURCES})
target_link_libraries(insightcad toolkit ${OCC_LIBRARIES})

install(TARGETS insightcad LIBRARY DESTINATION lib)

set(cadtest_SRCS test.cpp)
add_executable(cadtest ${cadtest_SRCS})
target_link_libraries(cadtest insightcad)
