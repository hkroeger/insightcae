
macro(install_headers NAME HEADERS)
  foreach (_hdr ${HEADERS})
      file (RELATIVE_PATH _relName "${CMAKE_CURRENT_LIST_DIR}" ${_hdr})
      get_filename_component(_pd ${_relName} DIRECTORY)
      add_custom_command(
          TARGET ${NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/insightcae/${_pd}
          COMMAND ${CMAKE_COMMAND} -E copy ${_hdr} ${CMAKE_BINARY_DIR}/include/insightcae/${_relName}
      )
      install(
        FILES ${_hdr}
        DESTINATION include/insightcae/${_pd}/
        COMPONENT ${INSIGHT_INSTALL_COMPONENT}
      )
 endforeach()
endmacro(install_headers)


## installs headers and PDL-generated headers
macro (add_PDL TARGETNAME FILES)
  #file(GLOB_RECURSE HEADERS "*.h")

  foreach (_hdrrel ${FILES})
    get_filename_component(_ext ${_hdrrel} EXT)
    if (_ext STREQUAL ".h")
        if (NOT IS_ABSOLUTE ${_hdrrel})
            set(_hdr ${CMAKE_CURRENT_SOURCE_DIR}/${_hdrrel})
        else()
            set(_hdr ${_hdrrel})
        endif()
        LIST(APPEND HEADERS ${_hdr})
    endif()
  endforeach()

  install_headers(${TARGETNAME} "${HEADERS}")

  foreach (_hdr ${HEADERS})
    #message(STATUS "${_hdr} ${_hdrrel}")
    get_filename_component(BN ${_hdr} NAME_WE)

    #message(STATUS ${_hdr} ${BN})
    list (APPEND ${TARGETNAME}_TIMESTAMPS ${BN}_pdl.timestamp)

    set(PDL pdl)
    set(GENSETSPY gen-sets.py)
    set(DEPS ${_hdr})
    if (IS_INSIGHTCAE_PROJECT AND NOT CMAKE_CROSSCOMPILING)
        set(GENSETSPY ${CMAKE_SOURCE_DIR}/gen-sets.py)
        set(PDL "${CMAKE_BINARY_DIR}/bin/pdl")
        list(APPEND DEPS ${PDL} ${GENSETSPY})
    endif()

    ADD_CUSTOM_COMMAND( OUTPUT ${BN}_pdl.timestamp
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/insightcae
                        COMMAND "${GENSETSPY}" "${_hdr}" "${PDL}" "${CMAKE_BINARY_DIR}/include/insightcae/"
                        COMMAND touch ${BN}_pdl.timestamp
                        DEPENDS ${DEPS}
                        COMMENT "Generating source code from PDL in header ${_hdr}" )
    install(
      CODE "file( GLOB _GeneratedHeaders \"${CMAKE_CURRENT_BINARY_DIR}/${BN}__*.h\" )"
      CODE "file( INSTALL \${_GeneratedHeaders} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/insightcae )"
      COMPONENT ${INSIGHT_INSTALL_COMPONENT}
     )
  endforeach()
  ADD_CUSTOM_TARGET( ${TARGETNAME}_PDLGenerator DEPENDS ${${TARGETNAME}_TIMESTAMPS}
                    COMMENT "Checking if PDL re-generation is required" )
  ADD_DEPENDENCIES( ${TARGETNAME} ${TARGETNAME}_PDLGenerator )

  target_include_directories(${TARGETNAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endmacro(add_PDL)
