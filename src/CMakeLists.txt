project(insight)


add_subdirectory(pdl)

IF(INSIGHT_BUILD_TOOLKIT)
  LIST(APPEND TARGETS_TO_INSTALL "InsightCAEToolkitTarget.cmake")
  add_subdirectory(toolkit)
  add_subdirectory(isGenerateParameterSetReference)
ENDIF()

add_subdirectory(refdata)
add_subdirectory(etc)
add_subdirectory(taskspooler)
add_subdirectory(extensions/python)
add_subdirectory(extensions/paraview)
add_subdirectory(extensions/taskjuggler)
add_subdirectory(extensions/dakota)

IF (INSIGHT_BUILD_CODEASTER)
    add_subdirectory(extensions/code_aster)
ENDIF()

if (INSIGHT_BUILD_OPENFOAM)
 add_subdirectory(extensions/openfoam)
ENDIF()

IF(INSIGHT_BUILD_TOOLKIT)
  add_subdirectory(toolkit_remote)
  LIST(APPEND TARGETS_TO_INSTALL "InsightCAEToolkitRemoteTarget.cmake")
  add_subdirectory(analyze)
  add_subdirectory(istmod)
  add_subdirectory(modules)

  if (INSIGHT_BUILD_GUICOMPONENTS)
    add_subdirectory(toolkit_gui)
    add_subdirectory(isresulttool)
    add_subdirectory(isparametersettool)
    LIST(APPEND TARGETS_TO_INSTALL "InsightCAEToolkitGUITarget.cmake")
    if (INSIGHT_BUILD_WORKBENCH)
     add_subdirectory(workbench)
    endif (INSIGHT_BUILD_WORKBENCH)
  endif (INSIGHT_BUILD_GUICOMPONENTS)

  IF(INSIGHT_BUILD_WEBWORKBENCH)
    add_subdirectory(webworkbench)
  ENDIF()

  if (INSIGHT_BUILD_CAD)
    add_subdirectory(cad)
    add_subdirectory(toolkit_cad)
    LIST(APPEND TARGETS_TO_INSTALL "InsightCAECADTarget.cmake")
    LIST(APPEND TARGETS_TO_INSTALL "InsightCAEToolkitCADTarget.cmake")
    #LIST(APPEND TARGETS_TO_INSTALL "InsightCAECADGUITarget.cmake")
  endif(INSIGHT_BUILD_CAD)

endif()


if (INSIGHT_BUILD_FREECAD)
  add_subdirectory(extensions/freecad)
endif (INSIGHT_BUILD_FREECAD)

IF (INSIGHT_BUILD_ADDONS)
  add_subdirectory(addons)
ENDIF()


#-- Add an Option to toggle the generation of the API documentation
if(BUILD_DOCUMENTATION)
  FIND_PACKAGE(Doxygen)
  if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR
      "Doxygen is needed to build the documentation. Please install it correctly")
  endif()
  #-- Configure the Template Doxyfile for our specific project
  configure_file(Doxyfile.in
                 ${PROJECT_BINARY_DIR}/Doxyfile  @ONLY IMMEDIATE)
  #-- Add a custom target to run Doxygen when ever the project is built
  add_custom_target (doc #ALL
                     COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                     SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
  # IF you do NOT want the documentation to be generated EVERY time you build the project
  # then leave out the 'ALL' keyword from the above command.
endif()

SET(TARGETS_TO_INSTALL ${TARGETS_TO_INSTALL} PARENT_SCOPE)
