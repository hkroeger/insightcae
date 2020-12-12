# 
# This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
# Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#

project(toolkit)

set(toolkit_SOURCES
    base/toolkitversion.cpp
    base/analysis.cpp
    base/parameter.cpp
    base/exception.cpp
    base/tools.cpp
    base/latextools.cpp
    base/linearalgebra.cpp
    base/resultelement.cpp
    base/resultelementcollection.cpp
    base/resultset.cpp
    base/global.cpp
    base/softwareenvironment.cpp
    base/stltools.cpp
    base/parameterset.cpp
    base/analysisstepcontrol.cpp
    base/plottools.cpp
    base/pythoninterface.cpp
    base/vtktools.cpp
    base/caseelement.cpp
    base/case.cpp
    base/units.cpp
    base/vtkrendering.cpp
    base/remotelocation.cpp
    base/remoteexecution.cpp
    base/remoteserverlist.cpp
    base/taskspoolerinterface.cpp
    base/mountremote.cpp
    base/outputanalyzer.cpp

    base/progressdisplayer.cpp

    base/progressdisplayer/textprogressdisplayer.cpp
    base/progressdisplayer/combinedprogressdisplayer.cpp
    base/progressdisplayer/prefixedprogressdisplayer.cpp
    base/progressdisplayer/convergenceanalysisdisplayer.cpp

    base/parameters/simpleparameter.cpp
    base/parameters/pathparameter.cpp
    base/parameters/selectionparameter.cpp
    base/parameters/doublerangeparameter.cpp
    base/parameters/arrayparameter.cpp
    base/parameters/matrixparameter.cpp

    base/resultelements/image.cpp
    base/resultelements/numericalresult.cpp
    base/resultelements/tabularresult.cpp
    base/resultelements/scalarresult.cpp
    base/resultelements/vectorresult.cpp
    base/resultelements/comment.cpp
    base/resultelements/attributeresulttable.cpp
    base/resultelements/resultsection.cpp
    base/resultelements/chart.cpp
    base/resultelements/chartrenderer.cpp
    base/resultelements/polarchart.cpp
    base/resultelements/contourchart.cpp
    
    openfoam/ofenvironment.cpp
    openfoam/ofes.cpp
    openfoam/blockmesh_templates.cpp
    openfoam/openfoamanalysis.cpp
    openfoam/openfoamcase.cpp
    openfoam/snappyhexmesh.cpp
    openfoam/cfmesh.cpp
    openfoam/openfoamdict.cpp
    openfoam/openfoamtools.cpp
    openfoam/blockmesh.cpp
    openfoam/fielddata.cpp
    openfoam/paraview.cpp
    openfoam/stretchtransformation.cpp
    openfoam/solveroutputanalyzer.cpp
    openfoam/blockmeshoutputanalyzer.cpp
    openfoam/snappyhexmeshoutputanalyzer.cpp
    openfoam/ofdicts.cpp


    openfoam/caseelements/openfoamcaseelement.cpp
    openfoam/caseelements/turbulencemodel.cpp

    openfoam/caseelements/basic/thermodynamicmodel.cpp
    openfoam/caseelements/basic/rasmodel.cpp
    openfoam/caseelements/basic/lesmodel.cpp
    openfoam/caseelements/basic/decomposepardict.cpp
    openfoam/caseelements/basic/cellsetoption_selection.cpp
    openfoam/caseelements/basic/gravity.cpp
    openfoam/caseelements/basic/minimumtimesteplimit.cpp
    openfoam/caseelements/basic/mirrormesh.cpp
    openfoam/caseelements/basic/setfieldsconfiguration.cpp
    openfoam/caseelements/basic/volumedrag.cpp
    openfoam/caseelements/basic/fixedvalueconstraint.cpp
    openfoam/caseelements/basic/source.cpp
    openfoam/caseelements/basic/mrfzone.cpp
    openfoam/caseelements/basic/passivescalar.cpp
    openfoam/caseelements/basic/pressuregradientsource.cpp
    openfoam/caseelements/basic/constantpressuregradientsource.cpp
    openfoam/caseelements/basic/transportmodel.cpp
    openfoam/caseelements/basic/singlephasetransportmodel.cpp
    openfoam/caseelements/basic/twophasetransportproperties.cpp
    openfoam/caseelements/basic/cavitationtwophasetransportproperties.cpp
    openfoam/caseelements/basic/porouszone.cpp
    openfoam/caseelements/basic/limitquantities.cpp
    openfoam/caseelements/basic/customdictentries.cpp
    openfoam/caseelements/basic/copyfiles.cpp
    openfoam/caseelements/basic/srfoption.cpp
    openfoam/caseelements/basic/providefields.cpp
    openfoam/caseelements/basic/rangeweightfield.cpp

    openfoam/caseelements/numerics/pimplesettings.cpp
    openfoam/caseelements/numerics/fvnumerics.cpp
    openfoam/caseelements/numerics/fanumerics.cpp
    openfoam/caseelements/numerics/tetfemnumerics.cpp
    openfoam/caseelements/numerics/meshingnumerics.cpp
    openfoam/caseelements/numerics/potentialfoamnumerics.cpp
    openfoam/caseelements/numerics/laplacianfoamnumerics.cpp
    openfoam/caseelements/numerics/steadyincompressiblenumerics.cpp
    openfoam/caseelements/numerics/unsteadyincompressiblenumerics.cpp
    openfoam/caseelements/numerics/steadycompressiblenumerics.cpp
    openfoam/caseelements/numerics/unsteadycompressiblenumerics.cpp
    openfoam/caseelements/numerics/potentialfreesurfacefoamnumerics.cpp
    openfoam/caseelements/numerics/simpledymfoamnumerics.cpp
    openfoam/caseelements/numerics/cavitatingfoamnumerics.cpp
    openfoam/caseelements/numerics/interfoamnumerics.cpp
    openfoam/caseelements/numerics/ltsinterfoamnumerics.cpp
    openfoam/caseelements/numerics/interphasechangefoamnumerics.cpp
    openfoam/caseelements/numerics/reactingfoamnumerics.cpp
    openfoam/caseelements/numerics/reactingparcelfoamnumerics.cpp
    openfoam/caseelements/numerics/buoyantsimplefoamnumerics.cpp
    openfoam/caseelements/numerics/buoyantpimplefoamnumerics.cpp
    openfoam/caseelements/numerics/fsidisplacementextrapolationnumerics.cpp
    openfoam/caseelements/numerics/magneticfoamnumerics.cpp

    openfoam/caseelements/boundarycondition.cpp
    openfoam/caseelements/boundaryconditions/boundarycondition_heat.cpp
    openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.cpp
    openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.cpp
    openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.cpp

    openfoam/caseelements/boundaryconditions/turbulentvelocityinletbc.cpp
    openfoam/caseelements/boundaryconditions/potentialfreesurfacebc.cpp
    openfoam/caseelements/boundaryconditions/wallbc.cpp
    openfoam/caseelements/boundaryconditions/pressureoutletbc.cpp
    openfoam/caseelements/boundaryconditions/simplebc.cpp
    openfoam/caseelements/boundaryconditions/symmetrybc.cpp
    openfoam/caseelements/boundaryconditions/emptybc.cpp
    openfoam/caseelements/boundaryconditions/cyclicpairbc.cpp
    openfoam/caseelements/boundaryconditions/ggibcbase.cpp
    openfoam/caseelements/boundaryconditions/ggibc.cpp
    openfoam/caseelements/boundaryconditions/cyclicggibc.cpp
    openfoam/caseelements/boundaryconditions/overlapggibc.cpp
    openfoam/caseelements/boundaryconditions/mixingplaneggibc.cpp
    openfoam/caseelements/boundaryconditions/suctioninletbc.cpp
    openfoam/caseelements/boundaryconditions/massflowbc.cpp
    openfoam/caseelements/boundaryconditions/mappedvelocityinletbc.cpp
    openfoam/caseelements/boundaryconditions/velocityinletbc.cpp
    openfoam/caseelements/boundaryconditions/exptdatainletbc.cpp
    openfoam/caseelements/boundaryconditions/compressibleinletbc.cpp

    openfoam/caseelements/dynamicmesh/dynamicmesh.cpp
    openfoam/caseelements/dynamicmesh/velocitytetfemmotionsolver.cpp
    openfoam/caseelements/dynamicmesh/displacementfvmotionsolver.cpp
    openfoam/caseelements/dynamicmesh/solidbodymotiondynamicmesh.cpp
    openfoam/caseelements/dynamicmesh/rigidbodymotiondynamicmesh.cpp

    openfoam/caseelements/turbulencemodelcaseelements.cpp
    openfoam/caseelements/analysiscaseelements.cpp
    openfoam/caseelements/thermophysicalcaseelements.cpp
    openfoam/caseelements/electromagneticscaseelements.cpp

    code_aster/caexportfile.cpp
    code_aster/codeasterrun.cpp
)

if( "${INSIGHT_CHARTRENDERER}" STREQUAL "gnuplot" )
    list(APPEND toolkit_SOURCES base/resultelements/gnuplotrenderer.cpp)
    add_definitions(-DCHART_RENDERER_GNUPLOT)
elseif("${INSIGHT_CHARTRENDERER}" STREQUAL "matplotlib" )
    list(APPEND toolkit_SOURCES base/resultelements/matplotlibrenderer.cpp)
    add_definitions(-DCHART_RENDERER_MATPLOTLIB)
endif()

IF(INSIGHT_BUILD_PYTHONBINDINGS)
  LIST(APPEND toolkit_SOURCES
   base/pythonanalysis.cpp
  )
ENDIF()


find_package(OpenSSL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(poppler-cpp REQUIRED IMPORTED_TARGET poppler-cpp)

file(GLOB_RECURSE toolkit_HEADERS "*.h")

link_directories(${Boost_LIBRARY_DIR})
add_library(toolkit SHARED ${toolkit_SOURCES} ${toolkit_HEADERS})
#set_target_properties(toolkit PROPERTIES LINK_FLAGS "-Wl,--no-as-needed")

target_link_libraries(toolkit
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
    ${ARMADILLO_LIBRARY}
    PkgConfig::poppler-cpp
    GSL::gsl GSL::gslcblas
    OpenSSL::SSL
    vtkHeaders
)

target_include_directories(toolkit
  PUBLIC ${PYTHON_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${ARMADILLO_INCLUDE_DIRS}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )

install_shared_file(gri_species openfoam/caseelements/gri.species thermophysical)

add_PDL(toolkit "${toolkit_HEADERS}")

install(
    TARGETS toolkit EXPORT toolkit_targets
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
export(EXPORT toolkit_targets
  FILE "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEToolkitTarget.cmake"
  NAMESPACE insight::
)
install(EXPORT toolkit_targets
  FILE InsightCAEToolkitTarget.cmake
  NAMESPACE insight::
  DESTINATION ${INSIGHT_ConfigPackageLocation}
)


IF(INSIGHT_BUILD_PYTHONBINDINGS)
    ADD_CUSTOM_COMMAND(
        OUTPUT toolkit_swigpyrun.timestamp
        COMMAND "${SWIG_EXECUTABLE}" -python -external-runtime
        COMMAND touch toolkit_swigpyrun.timestamp
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    ADD_CUSTOM_TARGET( toolkit_swigpyrun DEPENDS toolkit_swigpyrun.timestamp
            COMMENT "Checking for swigpyrun.h generation" )
    ADD_DEPENDENCIES( toolkit toolkit_swigpyrun )
ENDIF()



## pull-in modules for VTK on/offscreen in toolkit python wrapper
IF(INSIGHT_BUILD_PYTHONBINDINGS)
  foreach(_variant Offscreen Onscreen)
      set(TARGETNAME vtkPy${_variant})
      set(PYTARGET ${TARGETNAME}_python)
      set(IFFILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGETNAME}.i)
      set_property(SOURCE ${IFFILE} PROPERTY CPLUSPLUS ON)
      set_property(SOURCE ${IFFILE} PROPERTY SWIG_MODULE_NAME ${TARGETNAME})

      file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${TARGETNAME}.i "
%module ${TARGETNAME}
%{
#include <iostream>
%}
%pythonbegin %{
import os,sys
sys.setdlopenflags(os.RTLD_LAZY | os.RTLD_GLOBAL)
%}
")

      swig_add_library(${PYTARGET}
          LANGUAGE python
          SOURCES ${IFFILE})
      target_link_libraries(${PYTARGET} vtk${_variant})
      set_target_properties(${PYTARGET}
          PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR}
          )
    #  set_target_properties(${PYTARGET}
    #      PROPERTIES LINK_WHAT_YOU_USE TRUE # has no effect...
    #      )
      SET_TARGET_PROPERTIES(${PYTARGET} PROPERTIES LINK_FLAGS "-Wl,--no-as-needed")
      INSTALL ( FILES ${CMAKE_SWIG_OUTDIR}/_${PYTARGET}.so DESTINATION ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight COMPONENT ${INSIGHT_INSTALL_COMPONENT})
      INSTALL ( FILES ${CMAKE_SWIG_OUTDIR}/${TARGETNAME}.py DESTINATION ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight COMPONENT ${INSIGHT_INSTALL_COMPONENT})
  endforeach()
ENDIF()

add_pybindings(toolkit "toolkit.i" toolkit)


add_subdirectory(openfoam/tests)

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_RUNTIME} DESTINATION include/toolkit)
