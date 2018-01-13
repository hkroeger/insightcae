project(webworkbench)
cmake_minimum_required(VERSION 2.8)

find_package(Wt REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

set(webworkbench_SRCS
 webworkbench.cpp 
 insightanalysis.cpp
 insightsession.cpp
 inputform.cpp
 results.cpp
 analysisselector.cpp
 webparameterwrapper.cpp
)

INCLUDE_DIRECTORIES(${WT_INCLUDE_DIR})
ADD_EXECUTABLE(webworkbench ${webworkbench_SRCS})
TARGET_LINK_LIBRARIES(webworkbench
    toolkit
    ${Boost_LIBRARIES}
    ${Wt_DEBUG_LIBRARY}        # or {Wt_LIBRARY}
    ${Wt_HTTP_DEBUG_LIBRARY}   # or {Wt_HTTP_LIBRARY}
    ${Wt_EXT_DEBUG_LIBRARY}    # or {Wt_EXT_LIBRARY}
)

install(TARGETS webworkbench RUNTIME DESTINATION bin)
#install(FILES ${toolkit_HEADERS} DESTINATION include)
