
# Capture the directory of this file at include time so macros can reference
# sibling scripts regardless of where the macro is called from.
set(_INSIGHT_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

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
macro (target_add_PDL TARGETNAME)
  get_target_property(FILES ${TARGETNAME} SOURCES)
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
  # Write current header basenames so the cleanup script can detect removed/renamed headers
  set(_pdl_basenames "")
  foreach(_hdr ${HEADERS})
      get_filename_component(_bn ${_hdr} NAME_WE)
      list(APPEND _pdl_basenames ${_bn})
  endforeach()
  string(REPLACE ";" "\n" _pdl_basenames_str "${_pdl_basenames}")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${TARGETNAME}_pdl_headers.txt" "${_pdl_basenames_str}\n")

  ADD_CUSTOM_TARGET( ${TARGETNAME}_PDLGenerator
                    COMMAND ${CMAKE_COMMAND}
                        -DTARGET_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                        -DTARGET_NAME=${TARGETNAME}
                        -DINCLUDE_MIRROR_DIR=${CMAKE_BINARY_DIR}/include/insightcae
                        -P ${_INSIGHT_CMAKE_DIR}/pdl_stale_cleanup.cmake
                    DEPENDS ${${TARGETNAME}_TIMESTAMPS}
                    COMMENT "Checking if PDL re-generation is required" )
  ADD_DEPENDENCIES( ${TARGETNAME} ${TARGETNAME}_PDLGenerator )

  target_include_directories(${TARGETNAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endmacro(target_add_PDL)
