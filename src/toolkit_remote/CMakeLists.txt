project(toolkit_remote)


set(toolkit_remote_SRCS
    analyzeclient.cpp analyzeclient.h
    analyzeserverdetector.cpp analyzeserverdetector.h
    detectionhandler.cpp detectionhandler.h
    remoteparaview.cpp remoteparaview.h
)

if (wt_FOUND)
    message(STATUS "Found wt")

    add_library(toolkit_remote SHARED ${toolkit_remote_SRCS})
    add_PDL(toolkit_remote "${toolkit_remote_SRCS}")

    target_link_libraries(toolkit_remote
     toolkit
     Wt::Wt Wt::HTTP
     ${Boost_LIBRARIES}
     )

    target_include_directories(toolkit_remote
      PUBLIC ${Boost_INCLUDE_DIR}
      PUBLIC ${wt_INCLUDE_DIR}
      PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
      PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
     )

     if (INSIGHT_DEVELOPMENT_FILES)
        install(TARGETS toolkit_remote EXPORT toolkit_remote_targets
          LIBRARY DESTINATION lib
          COMPONENT ${INSIGHT_INSTALL_COMPONENT}
        )
        export(EXPORT toolkit_remote_targets
          FILE "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEToolkitRemoteTarget.cmake"
          NAMESPACE insight::
        )
        install(EXPORT toolkit_remote_targets
          FILE InsightCAEToolkitRemoteTarget.cmake
          NAMESPACE insight::
          DESTINATION ${INSIGHT_ConfigPackageLocation}
        )
    else()
	    install(TARGETS toolkit_remote LIBRARY DESTINATION lib)
    endif()


    add_subdirectory(tests)

else()
    message(STATUS "Did not found wt, disabling toolkit remote interface lib.")
endif()
