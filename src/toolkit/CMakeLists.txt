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
    base/cppextensions.h base/cppextensions.cpp
    base/translations.h base/translations.cpp
    base/global.cpp
    base/boost_include.cpp
    base/rapidxml.h base/rapidxml.cpp
    base/toolkitversion.cpp base/toolkitversion.h
    base/globalconfiguration.h base/globalconfiguration.cpp
    base/externalprograms.h base/externalprograms.cpp
    base/supplementedinputdata.cpp base/supplementedinputdata.h
    base/analysis.cpp base/analysis.h
    base/analysislibrary.cpp base/analysislibrary.h
    base/analysisparameterpropositions.h base/analysisparameterpropositions.cpp
    base/extendedgrammar.h base/extendedgrammar.cpp

    base/analysisthread.cpp base/analysisthread.h
    base/cacheableentity.cpp base/cacheableentity.h
    base/cacheableentityhashes.cpp base/cacheableentityhashes.h

    base/parameter.cpp base/parameter.h
    base/parameters/simpleparameter.cpp base/parameters/simpleparameter.h
    base/parameters/pathparameter.cpp base/parameters/pathparameter.h
    base/parameters/selectionparameter.cpp base/parameters/selectionparameter.h
    base/parameters/doublerangeparameter.cpp base/parameters/doublerangeparameter.h
    base/parameters/arrayparameter.cpp base/parameters/arrayparameter.h
    base/parameters/matrixparameter.cpp base/parameters/matrixparameter.h
    base/parameters/simpledimensionedparameter.cpp base/parameters/simpledimensionedparameter.h
    base/parameters/subsetparameter.cpp base/parameters/subsetparameter.h
    base/parameters/selectablesubsetparameter.cpp base/parameters/selectablesubsetparameter.h
    base/parameters/propertylibraryselectionparameter.h base/parameters/propertylibraryselectionparameter.cpp
    base/parameters/spatialtransformationparameter.h base/parameters/spatialtransformationparameter.cpp
    base/parameters/labeledarrayparameter.h base/parameters/labeledarrayparameter.cpp

    base/parameterset.cpp base/parameterset.h
    base/parametersetvisualizer.h base/parametersetvisualizer.cpp

    base/exception.cpp base/exception.h
    base/externalprocess.cpp base/externalprocess.h
    base/filewatcher.cpp base/filewatcher.h
    base/tools.cpp base/tools.h
    base/shelltools.h base/shelltools.cpp
    base/latextools.cpp base/latextools.h
    base/linearalgebra.cpp base/linearalgebra.h
    base/resultelement.cpp base/resultelement.h
    base/resultelementcollection.cpp base/resultelementcollection.h
    base/resultset.cpp base/resultset.h
    base/resultreporttemplates.h base/resultreporttemplates.cpp
    base/softwareenvironment.cpp base/softwareenvironment.h
    base/stltools.cpp base/stltools.h
    base/analysisstepcontrol.cpp base/analysisstepcontrol.h
    base/plottools.cpp base/plottools.h
    base/vtktools.cpp base/vtktools.h
    base/caseelement.cpp base/caseelement.h
    base/case.cpp base/case.h
    base/units.cpp base/units.h
    base/vtkrendering.cpp base/vtkrendering.h
    base/remotelocation.cpp base/remotelocation.h
    base/remoteexecution.cpp base/remoteexecution.h
    base/remoteserverlist.cpp base/remoteserverlist.h
    base/remoteserver.cpp base/remoteserver.h
    base/linuxremoteserver.cpp base/linuxremoteserver.h
    base/sshlinuxserver.cpp base/sshlinuxserver.h
    base/wsllinuxserver.cpp base/wsllinuxserver.h
    base/taskspoolerinterface.cpp base/taskspoolerinterface.h
    base/mountremote.cpp base/mountremote.h
    base/outputanalyzer.cpp base/outputanalyzer.h
    base/filecontainer.cpp base/filecontainer.h
    base/streamredirector.h base/streamredirector.cpp
    base/streamtoprogressdisplayer.h base/streamtoprogressdisplayer.cpp
    base/progressdisplayer.cpp base/progressdisplayer.h
    base/casedirectory.h base/casedirectory.cpp
    base/table.h base/table.cpp
    base/xmlfile.h base/xmlfile.cpp
    base/intervals.h base/intervals.cpp
    base/spatialtransformation.h base/spatialtransformation.cpp
    base/vtktransformation.h base/vtktransformation.cpp
    base/progressdisplayer/textprogressdisplayer.cpp base/progressdisplayer/textprogressdisplayer.h
    base/progressdisplayer/combinedprogressdisplayer.cpp base/progressdisplayer/combinedprogressdisplayer.h
    base/progressdisplayer/prefixedprogressdisplayer.cpp base/progressdisplayer/prefixedprogressdisplayer.h
    base/progressdisplayer/convergenceanalysisdisplayer.cpp base/progressdisplayer/convergenceanalysisdisplayer.h
    base/resultelements/fileresult.cpp base/resultelements/fileresult.h
    base/resultelements/image.cpp base/resultelements/image.h
    base/resultelements/video.cpp base/resultelements/video.h
    base/resultelements/numericalresult.cpp base/resultelements/numericalresult.h
    base/resultelements/tabularresult.cpp base/resultelements/tabularresult.h
    base/resultelements/scalarresult.cpp base/resultelements/scalarresult.h
    base/resultelements/vectorresult.cpp base/resultelements/vectorresult.h
    base/resultelements/comment.cpp base/resultelements/comment.h
    base/resultelements/attributeresulttable.cpp base/resultelements/attributeresulttable.h
    base/resultelements/resultsection.cpp base/resultelements/resultsection.h
    base/resultelements/chart.cpp base/resultelements/chart.h
    base/resultelements/chartrenderer.cpp base/resultelements/chartrenderer.h
    base/resultelements/polarchart.cpp base/resultelements/polarchart.h
    base/resultelements/polarchartrenderer.cpp base/resultelements/polarchartrenderer.h
    base/resultelements/latexgnuplotrenderer.h base/resultelements/latexgnuplotrenderer.cpp
    base/resultelements/gnuplotpolarchartrenderer.cpp base/resultelements/gnuplotpolarchartrenderer.h
    base/resultelements/fastgnuplotrenderer.h base/resultelements/fastgnuplotrenderer.cpp
    base/resultelements/contourchart.cpp base/resultelements/contourchart.h
    base/resultsetfilter.h base/resultsetfilter.cpp
    base/propertylibrary.h base/propertylibrary.cpp
    base/materialslibrary.h base/materialslibrary.cpp
    base/zipfile.h base/zipfile.cpp
    openfoam/ofenvironment.cpp openfoam/ofenvironment.h
    openfoam/ofes.cpp openfoam/ofes.h
    openfoam/blockmesh_templates.cpp openfoam/blockmesh_templates.h
    openfoam/openfoamanalysis.cpp openfoam/openfoamanalysis.h
    openfoam/openfoamcase.cpp openfoam/openfoamcase.h
    openfoam/snappyhexmesh.cpp openfoam/snappyhexmesh.h
    openfoam/cfmesh.cpp openfoam/cfmesh.h
    openfoam/openfoamdict.cpp openfoam/openfoamdict.h
    openfoam/openfoamboundarydict.cpp openfoam/openfoamboundarydict.h
    openfoam/openfoamtools.cpp openfoam/openfoamtools.h
    openfoam/blockmesh/arcedge.cpp openfoam/blockmesh/arcedge.h
    openfoam/blockmesh/block2d.cpp openfoam/blockmesh/block2d.h
    openfoam/blockmesh/block.cpp openfoam/blockmesh/block.h
    openfoam/blockmesh/circularedge_center.cpp openfoam/blockmesh/circularedge_center.h
    openfoam/blockmesh/circularedge.cpp openfoam/blockmesh/circularedge.h
    openfoam/blockmesh/discretecurve.cpp openfoam/blockmesh/discretecurve.h
    openfoam/blockmesh/edge.cpp openfoam/blockmesh/edge.h
    openfoam/blockmesh/ellipseedge.cpp openfoam/blockmesh/ellipseedge.h
    openfoam/blockmesh/geometry.cpp openfoam/blockmesh/geometry.h
    openfoam/blockmesh/gradinganalyzer.cpp openfoam/blockmesh/gradinganalyzer.h
    openfoam/blockmesh/patch2d.cpp openfoam/blockmesh/patch2d.h
    openfoam/blockmesh/patch.cpp openfoam/blockmesh/patch.h
    openfoam/blockmesh/plane2d.cpp openfoam/blockmesh/plane2d.h
    openfoam/blockmesh/point.cpp openfoam/blockmesh/point.h
    openfoam/blockmesh/projectededge.cpp openfoam/blockmesh/projectededge.h
    openfoam/blockmesh/projectedface.cpp openfoam/blockmesh/projectedface.h
    openfoam/blockmesh/splineedge.cpp openfoam/blockmesh/splineedge.h
    openfoam/blockmesh/transform2d.cpp openfoam/blockmesh/transform2d.h
    openfoam/blockmesh/wedge2d.cpp openfoam/blockmesh/wedge2d.h
    openfoam/blockmesh.cpp openfoam/blockmesh.h
    openfoam/fielddata.cpp openfoam/fielddata.h
    openfoam/paraview.cpp openfoam/paraview.h
    openfoam/stretchtransformation.cpp openfoam/stretchtransformation.h
    openfoam/solveroutputanalyzer.cpp openfoam/solveroutputanalyzer.h
    openfoam/blockmeshoutputanalyzer.cpp openfoam/blockmeshoutputanalyzer.h
    openfoam/snappyhexmeshoutputanalyzer.cpp openfoam/snappyhexmeshoutputanalyzer.h
    openfoam/ofdicts.cpp openfoam/ofdicts.h
    openfoam/setfields.h openfoam/setfields.cpp
    openfoam/createpatch.h openfoam/createpatch.cpp
    openfoam/caseelements/openfoamcaseelement.cpp openfoam/caseelements/openfoamcaseelement.h
    openfoam/caseelements/turbulencemodel.cpp openfoam/caseelements/turbulencemodel.h

    openfoam/caseelements/basic/thermodynamicmodel.cpp openfoam/caseelements/basic/thermodynamicmodel.h
    openfoam/caseelements/basic/rasmodel.cpp openfoam/caseelements/basic/rasmodel.h
    openfoam/caseelements/basic/lesmodel.cpp openfoam/caseelements/basic/lesmodel.h
    openfoam/caseelements/basic/decomposepardict.cpp openfoam/caseelements/basic/decomposepardict.h
    openfoam/caseelements/basic/cellsetoption_selection.cpp openfoam/caseelements/basic/cellsetoption_selection.h
    openfoam/caseelements/basic/gravity.cpp openfoam/caseelements/basic/gravity.h
    openfoam/caseelements/basic/minimumtimesteplimit.cpp openfoam/caseelements/basic/minimumtimesteplimit.h
    openfoam/caseelements/basic/mirrormesh.cpp openfoam/caseelements/basic/mirrormesh.h
    openfoam/caseelements/basic/setfieldsconfiguration.cpp openfoam/caseelements/basic/setfieldsconfiguration.h
    openfoam/caseelements/basic/volumedrag.cpp openfoam/caseelements/basic/volumedrag.h
    openfoam/caseelements/basic/fixedvalueconstraint.cpp openfoam/caseelements/basic/fixedvalueconstraint.h
    openfoam/caseelements/basic/source.cpp openfoam/caseelements/basic/source.h
    openfoam/caseelements/basic/mrfzone.cpp openfoam/caseelements/basic/mrfzone.h
    openfoam/caseelements/basic/passivescalar.cpp openfoam/caseelements/basic/passivescalar.h
    openfoam/caseelements/basic/pressuregradientsource.cpp openfoam/caseelements/basic/pressuregradientsource.h
    openfoam/caseelements/basic/constantpressuregradientsource.cpp openfoam/caseelements/basic/constantpressuregradientsource.h
    openfoam/caseelements/basic/transportmodel.cpp openfoam/caseelements/basic/transportmodel.h
    openfoam/caseelements/basic/singlephasetransportmodel.cpp openfoam/caseelements/basic/singlephasetransportmodel.h
    openfoam/caseelements/basic/twophasetransportproperties.cpp openfoam/caseelements/basic/twophasetransportproperties.h
    openfoam/caseelements/basic/compressibletwophasethermophysicalproperties.h openfoam/caseelements/basic/compressibletwophasethermophysicalproperties.cpp
    openfoam/caseelements/basic/cavitationtwophasetransportproperties.cpp openfoam/caseelements/basic/cavitationtwophasetransportproperties.h
    openfoam/caseelements/basic/porouszone.cpp openfoam/caseelements/basic/porouszone.h
    openfoam/caseelements/basic/limitquantities.cpp openfoam/caseelements/basic/limitquantities.h
    openfoam/caseelements/basic/customdictentries.cpp openfoam/caseelements/basic/customdictentries.h
    openfoam/caseelements/basic/copyfiles.cpp openfoam/caseelements/basic/copyfiles.h
    openfoam/caseelements/basic/srfoption.cpp openfoam/caseelements/basic/srfoption.h
    openfoam/caseelements/basic/providefields.cpp openfoam/caseelements/basic/providefields.h
    openfoam/caseelements/basic/rangeweightfield.cpp openfoam/caseelements/basic/rangeweightfield.h
    openfoam/caseelements/basic/wallheatflux.cpp openfoam/caseelements/basic/wallheatflux.h

    openfoam/caseelements/numerics/oversetconfiguration.cpp openfoam/caseelements/numerics/oversetconfiguration.h
    openfoam/caseelements/numerics/pimplesettings.cpp openfoam/caseelements/numerics/pimplesettings.h
    openfoam/caseelements/numerics/fvnumerics.cpp openfoam/caseelements/numerics/fvnumerics.h
    openfoam/caseelements/numerics/fanumerics.cpp openfoam/caseelements/numerics/fanumerics.h
    openfoam/caseelements/numerics/tetfemnumerics.cpp openfoam/caseelements/numerics/tetfemnumerics.h
    openfoam/caseelements/numerics/meshingnumerics.cpp openfoam/caseelements/numerics/meshingnumerics.h
    openfoam/caseelements/numerics/potentialfoamnumerics.cpp openfoam/caseelements/numerics/potentialfoamnumerics.h
    openfoam/caseelements/numerics/laplacianfoamnumerics.cpp openfoam/caseelements/numerics/laplacianfoamnumerics.h
    openfoam/caseelements/numerics/steadyincompressiblenumerics.cpp openfoam/caseelements/numerics/steadyincompressiblenumerics.h
    openfoam/caseelements/numerics/unsteadyincompressiblenumerics.cpp openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h
    openfoam/caseelements/numerics/steadycompressiblenumerics.cpp openfoam/caseelements/numerics/steadycompressiblenumerics.h
    openfoam/caseelements/numerics/unsteadycompressiblenumerics.cpp openfoam/caseelements/numerics/unsteadycompressiblenumerics.h
    openfoam/caseelements/numerics/potentialfreesurfacefoamnumerics.cpp openfoam/caseelements/numerics/potentialfreesurfacefoamnumerics.h
    openfoam/caseelements/numerics/scalartransportfoamnumerics.cpp openfoam/caseelements/numerics/scalartransportfoamnumerics.h
    openfoam/caseelements/numerics/simpledymfoamnumerics.cpp openfoam/caseelements/numerics/simpledymfoamnumerics.h
    openfoam/caseelements/numerics/cavitatingfoamnumerics.cpp openfoam/caseelements/numerics/cavitatingfoamnumerics.h
    openfoam/caseelements/numerics/interfoamnumerics.cpp openfoam/caseelements/numerics/interfoamnumerics.h
    openfoam/caseelements/numerics/compressibleinterfoamnumerics.h openfoam/caseelements/numerics/compressibleinterfoamnumerics.cpp
    openfoam/caseelements/numerics/ltsinterfoamnumerics.cpp openfoam/caseelements/numerics/ltsinterfoamnumerics.h
    openfoam/caseelements/numerics/interphasechangefoamnumerics.cpp openfoam/caseelements/numerics/interphasechangefoamnumerics.h
    openfoam/caseelements/numerics/reactingfoamnumerics.cpp openfoam/caseelements/numerics/reactingfoamnumerics.h
    openfoam/caseelements/numerics/reactingparcelfoamnumerics.cpp openfoam/caseelements/numerics/reactingparcelfoamnumerics.h
    openfoam/caseelements/numerics/buoyantsimplefoamnumerics.cpp openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h
    openfoam/caseelements/numerics/buoyantpimplefoamnumerics.cpp openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h
    openfoam/caseelements/numerics/fsidisplacementextrapolationnumerics.cpp openfoam/caseelements/numerics/fsidisplacementextrapolationnumerics.h
    openfoam/caseelements/numerics/magneticfoamnumerics.cpp openfoam/caseelements/numerics/magneticfoamnumerics.h
    openfoam/caseelements/numerics/chtmultiregionnumerics.cpp openfoam/caseelements/numerics/chtmultiregionnumerics.h

    openfoam/caseelements/boundarycondition.cpp openfoam/caseelements/boundarycondition.h
    openfoam/caseelements/boundaryconditions/boundarycondition_heat.cpp openfoam/caseelements/boundaryconditions/boundarycondition_heat.h
    openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.cpp openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h
    openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.cpp openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h
    openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.cpp openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h
    openfoam/caseelements/boundaryconditions/turbulentvelocityinletbc.cpp openfoam/caseelements/boundaryconditions/turbulentvelocityinletbc.h
    openfoam/caseelements/boundaryconditions/potentialfreesurfacebc.cpp openfoam/caseelements/boundaryconditions/potentialfreesurfacebc.h
    openfoam/caseelements/boundaryconditions/wallbc.cpp openfoam/caseelements/boundaryconditions/wallbc.h
    openfoam/caseelements/boundaryconditions/pressureoutletbc.cpp openfoam/caseelements/boundaryconditions/pressureoutletbc.h
    openfoam/caseelements/boundaryconditions/simplebc.cpp openfoam/caseelements/boundaryconditions/simplebc.h
    openfoam/caseelements/boundaryconditions/symmetrybc.cpp openfoam/caseelements/boundaryconditions/symmetrybc.h
    openfoam/caseelements/boundaryconditions/emptybc.cpp openfoam/caseelements/boundaryconditions/emptybc.h
    openfoam/caseelements/boundaryconditions/cyclicpairbc.cpp openfoam/caseelements/boundaryconditions/cyclicpairbc.h
    openfoam/caseelements/boundaryconditions/ggibcbase.cpp openfoam/caseelements/boundaryconditions/ggibcbase.h
    openfoam/caseelements/boundaryconditions/ggibc.cpp openfoam/caseelements/boundaryconditions/ggibc.h
    openfoam/caseelements/boundaryconditions/cyclicggibc.cpp openfoam/caseelements/boundaryconditions/cyclicggibc.h
    openfoam/caseelements/boundaryconditions/overlapggibc.cpp openfoam/caseelements/boundaryconditions/overlapggibc.h
    openfoam/caseelements/boundaryconditions/mixingplaneggibc.cpp openfoam/caseelements/boundaryconditions/mixingplaneggibc.h
    openfoam/caseelements/boundaryconditions/suctioninletbc.cpp openfoam/caseelements/boundaryconditions/suctioninletbc.h
    openfoam/caseelements/boundaryconditions/massflowbc.cpp openfoam/caseelements/boundaryconditions/massflowbc.h
    openfoam/caseelements/boundaryconditions/mappedvelocityinletbc.cpp openfoam/caseelements/boundaryconditions/mappedvelocityinletbc.h
    openfoam/caseelements/boundaryconditions/velocityinletbc.cpp openfoam/caseelements/boundaryconditions/velocityinletbc.h
    openfoam/caseelements/boundaryconditions/exptdatainletbc.cpp openfoam/caseelements/boundaryconditions/exptdatainletbc.h
    openfoam/caseelements/boundaryconditions/compressibleinletbc.cpp openfoam/caseelements/boundaryconditions/compressibleinletbc.h
    openfoam/caseelements/boundaryconditions/cyclicacmibc.h openfoam/caseelements/boundaryconditions/cyclicacmibc.cpp

    openfoam/caseelements/dynamicmesh/sixdofrigidbodymotionsolver.h openfoam/caseelements/dynamicmesh/sixdofrigidbodymotionsolver.cpp
    openfoam/caseelements/dynamicmesh/dynamicmesh.cpp openfoam/caseelements/dynamicmesh/dynamicmesh.h
    openfoam/caseelements/dynamicmesh/velocitytetfemmotionsolver.cpp openfoam/caseelements/dynamicmesh/velocitytetfemmotionsolver.h
    openfoam/caseelements/dynamicmesh/displacementfvmotionsolver.cpp openfoam/caseelements/dynamicmesh/displacementfvmotionsolver.h
    openfoam/caseelements/dynamicmesh/solidbodymotiondynamicmesh.cpp openfoam/caseelements/dynamicmesh/solidbodymotiondynamicmesh.h
    openfoam/caseelements/dynamicmesh/rigidbodymotiondynamicmesh.cpp openfoam/caseelements/dynamicmesh/rigidbodymotiondynamicmesh.h
    openfoam/caseelements/dynamicmesh/dynamicoversetfvmesh.h openfoam/caseelements/dynamicmesh/dynamicoversetfvmesh.cpp

    openfoam/caseelements/turbulencemodels/laminar_rasmodel.cpp openfoam/caseelements/turbulencemodels/laminar_rasmodel.h
    openfoam/caseelements/turbulencemodels/smagorinsky_lesmodel.cpp openfoam/caseelements/turbulencemodels/smagorinsky_lesmodel.h
    openfoam/caseelements/turbulencemodels/oneeqeddy_lesmodel.cpp openfoam/caseelements/turbulencemodels/oneeqeddy_lesmodel.h
    openfoam/caseelements/turbulencemodels/dynoneeqeddy_lesmodel.cpp openfoam/caseelements/turbulencemodels/dynoneeqeddy_lesmodel.h
    openfoam/caseelements/turbulencemodels/dynsmagorinsky_lesmodel.cpp openfoam/caseelements/turbulencemodels/dynsmagorinsky_lesmodel.h
    openfoam/caseelements/turbulencemodels/komegasst_rasmodel.cpp openfoam/caseelements/turbulencemodels/komegasst_rasmodel.h
    openfoam/caseelements/turbulencemodels/kepsilonbase_rasmodel.cpp openfoam/caseelements/turbulencemodels/kepsilonbase_rasmodel.h
    openfoam/caseelements/turbulencemodels/kepsilon_rasmodel.cpp openfoam/caseelements/turbulencemodels/kepsilon_rasmodel.h
    openfoam/caseelements/turbulencemodels/realizablekepsilon_rasmodel.cpp openfoam/caseelements/turbulencemodels/realizablekepsilon_rasmodel.h
    openfoam/caseelements/turbulencemodels/spalartallmaras_rasmodel.cpp openfoam/caseelements/turbulencemodels/spalartallmaras_rasmodel.h
    openfoam/caseelements/turbulencemodels/lemoshybrid_rasmodel.cpp openfoam/caseelements/turbulencemodels/lemoshybrid_rasmodel.h
    openfoam/caseelements/turbulencemodels/komegasst_lowre_rasmodel.cpp openfoam/caseelements/turbulencemodels/komegasst_lowre_rasmodel.h
    openfoam/caseelements/turbulencemodels/komegasst2_rasmodel.cpp openfoam/caseelements/turbulencemodels/komegasst2_rasmodel.h
    openfoam/caseelements/turbulencemodels/komegahe_rasmodel.cpp openfoam/caseelements/turbulencemodels/komegahe_rasmodel.h
    openfoam/caseelements/turbulencemodels/lrr_rasmodel.cpp openfoam/caseelements/turbulencemodels/lrr_rasmodel.h
    openfoam/caseelements/turbulencemodels/wale_lesmodel.cpp openfoam/caseelements/turbulencemodels/wale_lesmodel.h
    openfoam/caseelements/analysiscaseelements.cpp openfoam/caseelements/analysiscaseelements.h
    openfoam/caseelements/thermophysicalcaseelements.cpp openfoam/caseelements/thermophysicalcaseelements.h
    openfoam/caseelements/electromagneticscaseelements.cpp openfoam/caseelements/electromagneticscaseelements.h
    openfoam/caseelements/options/enforcefarfieldvelocity.h openfoam/caseelements/options/enforcefarfieldvelocity.cpp


    code_aster/caexportfile.cpp code_aster/caexportfile.h
    code_aster/codeasterrun.cpp code_aster/codeasterrun.h
    code_aster/codeastermeshfile.h code_aster/codeastermeshfile.cpp
    code_aster/casolveroutputanalyzer.cpp code_aster/casolveroutputanalyzer.h
    code_aster/coordinatesystems.h code_aster/coordinatesystems.cpp

    lsdyna/lsdynainputdeck.h lsdyna/lsdynainputdeck.cpp
    lsdyna/lsdynainputcard.h lsdyna/lsdynainputcard.cpp
    lsdyna/control.h lsdyna/control.cpp
    lsdyna/mesh.h lsdyna/mesh.cpp
    lsdyna/materials.h lsdyna/materials.cpp
    lsdyna/solution.h lsdyna/solution.cpp
    lsdyna/boundaryconditions.h lsdyna/boundaryconditions.cpp
    lsdyna/contact.h lsdyna/contact.cpp
    lsdyna/radsoftwareenvironment.h lsdyna/radsoftwareenvironment.cpp
    lsdyna/openradiossoutputanalyzer.h lsdyna/openradiossoutputanalyzer.cpp
)

if (INSIGHT_GNUPLOT_CHARTRENDERER)
    list(APPEND toolkit_SOURCES base/resultelements/gnuplotrenderer.cpp base/resultelements/gnuplotrenderer.h)
    add_definitions(-DCHART_RENDERER_GNUPLOT)
endif()

if (INSIGHT_MATPLOTLIB_CHARTRENDERER)
    list(APPEND toolkit_SOURCES base/resultelements/matplotlibrenderer.cpp base/resultelements/matplotlibrenderer.h)
    add_definitions(-DCHART_RENDERER_MATPLOTLIB)
endif()

IF(INSIGHT_BUILD_PYTHONBINDINGS)
  LIST(APPEND toolkit_SOURCES
   base/pythonanalysis.cpp base/pythonanalysis.h
   base/pythoninterface.cpp base/pythoninterface.h
  )
ENDIF()


find_package(OpenSSL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(poppler-cpp REQUIRED IMPORTED_TARGET poppler-cpp)
PKG_CHECK_MODULES(minizip REQUIRED IMPORTED_TARGET minizip)


link_directories(${Boost_LIBRARY_DIR})
add_library(toolkit SHARED ${toolkit_SOURCES})
set(toolkit_LINK_LIBS
    ${Boost_LIBRARIES}
    ${Intl_LIBRARY}
    ${ARMADILLO_LIBRARY}
    PkgConfig::poppler-cpp
    PkgConfig::minizip
    GSL::gsl GSL::gslcblas
    OpenSSL::SSL
    vtkHeaders
)
if (PYTHONLIBS_FOUND)
    list(APPEND toolkit_LINK_LIBS ${PYTHON_LIBRARIES})
endif()
if (WIN32)
    list(APPEND toolkit_LINK_LIBS vtkOnscreen)
endif()
target_link_libraries(toolkit ${toolkit_LINK_LIBS} ${Boost_LIBRARIES})
# --export-all-symbols is not sufficient for signal/slots to work across DLLs
include(GenerateExportHeader)
generate_export_header(toolkit)
target_compile_definitions(toolkit PRIVATE toolkit_EXPORTS)

setup_gettext(toolkit toolkit ${toolkit_SOURCES})

install_shared_file2(base/materialLibrary.xml ".")

target_include_directories(toolkit
  PUBLIC ${PYTHON_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${Intl_INCLUDE_DIRS}
  PUBLIC ${ARMADILLO_INCLUDE_DIRS}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )
add_library(toolkitHeaders INTERFACE)
get_target_property(toolkit_INCLUDE_DIRS toolkit INCLUDE_DIRECTORIES)
target_include_directories(toolkitHeaders INTERFACE ${toolkit_INCLUDE_DIRS})




set(EXPORT_OPTS "-Wl,--no-as-needed -Wl,--copy-dt-needed-entries")

macro(linkToolkitVtk2Exe target OnOffscreen)
    target_link_libraries(${target} toolkit vtk${OnOffscreen} )
    set_target_properties(${target} PROPERTIES LINK_FLAGS ${EXPORT_OPTS})
endmacro()

macro(buildLibraryList VTK_LIB_LIST)
    set(LIBRARY_LIST "")
    foreach (_l ${VTK_LIB_LIST})
        get_filename_component(_ln ${_l} NAME)
        if (LIBRARY_LIST)
            set(LIBRARY_LIST "${LIBRARY_LIST}, \"${_ln}\"")
        else()
            set(LIBRARY_LIST "\"${_ln}\"")
        endif()
    endforeach()
    set(LIBRARY_LIST "${LIBRARY_LIST}, \"libtoolkit.so\"")
endmacro()


if (NOT WIN32)

    buildLibraryList("${VTK_OFFSCREEN_LIBRARIES}")
    configure_file(sharedLibraryLoader.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/toolkitVtkOffscreenLoader.cpp @ONLY)
    add_library(toolkitVtkOffscreenLoader SHARED toolkitVtkOffscreenLoader.cpp)
    add_dependencies(toolkitVtkOffscreenLoader vtkOffscreen)
    set_target_properties(toolkitVtkOffscreenLoader PROPERTIES LINK_FLAGS ${EXPORT_OPTS})
    install(
        TARGETS toolkitVtkOffscreenLoader # EXPORT toolkit_targets
        LIBRARY DESTINATION lib
        COMPONENT ${INSIGHT_INSTALL_COMPONENT}
        )

    buildLibraryList("${VTK_ONSCREEN_LIBRARIES}")
    configure_file(sharedLibraryLoader.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/toolkitVtkOnscreenLoader.cpp @ONLY)
    add_library(toolkitVtkOnscreenLoader SHARED toolkitVtkOnscreenLoader.cpp)
    add_dependencies(toolkitVtkOnscreenLoader vtkOnscreen)
    set_target_properties(toolkitVtkOnscreenLoader PROPERTIES LINK_FLAGS ${EXPORT_OPTS})
    install(
        TARGETS toolkitVtkOnscreenLoader # EXPORT toolkit_targets
        LIBRARY DESTINATION lib
        COMPONENT ${INSIGHT_INSTALL_COMPONENT}
        )

endif()

macro(linkToolkitVtk2ShLib target OnOffscreen)
    if (WIN32)
        target_link_libraries(${target} toolkit)
    else()
        target_link_libraries(${target} toolkitHeaders toolkitVtk${OnOffscreen}Loader )
    endif()
endmacro()

macro(linkToolkitVtk target OnOffscreen)
    if (WIN32)
        target_link_libraries(${target} toolkit)
    else()
        target_link_libraries(${target} toolkit vtk${OnOffscreen} )
        set_target_properties(${target} PROPERTIES LINK_FLAGS "-Wl,--no-as-needed -Wl,--copy-dt-needed-entries")
    endif()
endmacro()


macro(linkToolkit_OF_Exe target)
 version_list_OF()
 foreach (_ofvers ${OF_VERSIONS})
  if (${_ofvers}_FOUND)
   linkToolkitVtk(${target}${_ofvers} Offscreen)
  endif()
 endforeach()
endmacro()


macro(linkToolkit_OF_ShLib target)
 version_list_OF()
 foreach (_ofvers ${OF_VERSIONS})
  if (${_ofvers}_FOUND)
   linkToolkitVtk(${target}${_ofvers} Offscreen)
   #linkToolkitVtk2ShLib(${target}${_ofvers} Offscreen)
  endif()
 endforeach()
endmacro()




install_shared_file(gri_species openfoam/caseelements/gri.species thermophysical)

add_PDL(toolkit "${toolkit_SOURCES}")


if (INSIGHT_DEVELOPMENT_FILES)
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
else()
   install(
        TARGETS toolkit
	LIBRARY DESTINATION lib
	COMPONENT ${INSIGHT_INSTALL_COMPONENT}
        )
endif()

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
  if (NOT WIN32)
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
  endif()
ENDIF()

if (WIN32)
  add_pybindings(toolkit "toolkit_win32.i" toolkit)
else()
  add_pybindings(toolkit "toolkit.i" toolkit)
endif()


#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SWIG_RUNTIME} DESTINATION include/toolkit)
