
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
          COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_hdr} ${CMAKE_BINARY_DIR}/include/insightcae/${_relName}
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
  set(PDL pdl)
  set(GENSETSPY gen-sets.py)
  if (IS_INSIGHTCAE_PROJECT AND NOT CMAKE_CROSSCOMPILING)
      set(GENSETSPY ${CMAKE_SOURCE_DIR}/gen-sets.py)
      set(PDL "${CMAKE_BINARY_DIR}/bin/pdl")
      list(APPEND PDLDEPS ${PDL} ${GENSETSPY})
  endif()

  # find all headers in target's sources, store in HEADERS
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

  #mark headers for inclusion in installation
  install_headers(${TARGETNAME} "${HEADERS}")

  # examine each header for PDL code
  foreach (_hdr ${HEADERS})

    # basename into BN
    get_filename_component(BN ${_hdr} NAME_WE)

    list (APPEND ${TARGETNAME}_TIMESTAMPS ${BN}_pdl.timestamp)

    set(DEPS ${PDLDEPS})
    list(APPEND DEPS ${_hdr})

    # add rule on how to generate ${BN}_pdl.timestamp, if needed
    add_custom_command( OUTPUT ${BN}_pdl.timestamp
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

  add_custom_target( ${TARGETNAME}_PDLGenerator
                    COMMAND ${CMAKE_COMMAND}
                        -DTARGET_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                        -DTARGET_NAME=${TARGETNAME}
                        -DINCLUDE_MIRROR_DIR=${CMAKE_BINARY_DIR}/include/insightcae
                        -P ${_INSIGHT_CMAKE_DIR}/pdl_stale_cleanup.cmake
                    DEPENDS ${${TARGETNAME}_TIMESTAMPS}
                    COMMENT "Checking if PDL re-generation is required" )
  add_dependencies( ${TARGETNAME} ${TARGETNAME}_PDLGenerator )

  target_include_directories(${TARGETNAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endmacro(target_add_PDL)
