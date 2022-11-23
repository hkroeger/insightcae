/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


%module(directors="1") toolkit

%pythonbegin %{
import re, os
liblist=list(filter(re.compile('libvtkCommon.*so.*').search, [l[73:].strip() for l in open('/proc/%d/maps'%os.getpid(), 'r').readlines()]))
if len(liblist)==0:
  import Insight.vtkPyOffscreen
%}


%{
#include "base/boost_include.h"
#include "boost/filesystem/path.hpp"
#include "base/factory.h"
#include "base/parameter.h"
#include "base/parameterset.h"
#include "base/resultelement.h"
#include "base/resultelementcollection.h"
#include "base/resultset.h"
#include "base/resultelements/numericalresult.h"
#include "base/resultelements/scalarresult.h"
#include "base/resultelements/vectorresult.h"
#include "base/resultelements/tabularresult.h"
#include "base/resultelements/resultsection.h"
#include "base/resultelements/polarchart.h"
#include "base/resultelements/image.h"
#include "base/resultelements/contourchart.h"
#include "base/resultelements/comment.h"
#include "base/resultelements/chart.h"
#include "base/resultelements/attributeresulttable.h"
#include "base/analysis.h"
#include "base/pythonanalysis.h"
#include "base/parameterstudy.h"
#include "base/case.h"
#include "base/caseelement.h"
#include "base/linearalgebra.h"
#include "base/mountremote.h"
#include "base/taskspoolerinterface.h"
#include "base/remoteserverlist.h"
#include "base/remotelocation.h"
#include "base/remoteexecution.h"

#include "openfoam/blockmesh_templates.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/ofenvironment.h"
#include "openfoam/openfoamparameterstudy.h"
#include "openfoam/snappyhexmesh.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/stretchtransformation.h"
#include "openfoam/ofes.h"
#include "openfoam/paraview.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/laplacianfoamnumerics.h"
#include "openfoam/caseelements/numerics/cavitatingfoamnumerics.h"
#include "openfoam/caseelements/numerics/reactingfoamnumerics.h"
#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/numerics/interphasechangefoamnumerics.h"
#include "openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/potentialfreesurfacefoamnumerics.h"
#include "openfoam/caseelements/numerics/fsidisplacementextrapolationnumerics.h"
#include "openfoam/caseelements/numerics/pimplesettings.h"
#include "openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/tetfemnumerics.h"
#include "openfoam/caseelements/numerics/steadycompressiblenumerics.h"
#include "openfoam/caseelements/numerics/unsteadycompressiblenumerics.h"
#include "openfoam/caseelements/numerics/reactingparcelfoamnumerics.h"
#include "openfoam/caseelements/numerics/fanumerics.h"
#include "openfoam/caseelements/numerics/simpledymfoamnumerics.h"
#include "openfoam/caseelements/numerics/magneticfoamnumerics.h"
#include "openfoam/caseelements/numerics/interfoamnumerics.h"
#include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/potentialfoamnumerics.h"
#include "openfoam/caseelements/numerics/ltsinterfoamnumerics.h"
#include "openfoam/caseelements/thermophysicalcaseelements.h"
#include "openfoam/caseelements/turbulencemodels/smagorinsky_lesmodel.h"
#include "openfoam/caseelements/turbulencemodels/lrr_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/kepsilonbase_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/kepsilon_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/dynsmagorinsky_lesmodel.h"
#include "openfoam/caseelements/turbulencemodels/laminar_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/komegasst2_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/oneeqeddy_lesmodel.h"
#include "openfoam/caseelements/turbulencemodels/dynoneeqeddy_lesmodel.h"
#include "openfoam/caseelements/turbulencemodels/lemoshybrid_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/spalartallmaras_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/komegasst_lowre_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/komegasst_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/komegahe_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/realizablekepsilon_rasmodel.h"
#include "openfoam/caseelements/turbulencemodels/wale_lesmodel.h"

#include "openfoam/caseelements/electromagneticscaseelements.h"
#include "openfoam/caseelements/dynamicmesh/velocitytetfemmotionsolver.h"
#include "openfoam/caseelements/dynamicmesh/displacementfvmotionsolver.h"
#include "openfoam/caseelements/dynamicmesh/solidbodymotiondynamicmesh.h"
#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"
#include "openfoam/caseelements/dynamicmesh/rigidbodymotiondynamicmesh.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/ggibc.h"
#include "openfoam/caseelements/boundaryconditions/turbulentvelocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"
#include "openfoam/caseelements/boundaryconditions/cyclicggibc.h"
#include "openfoam/caseelements/boundaryconditions/mappedvelocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/symmetrybc.h"
#include "openfoam/caseelements/boundaryconditions/potentialfreesurfacebc.h"
#include "openfoam/caseelements/boundaryconditions/overlapggibc.h"
#include "openfoam/caseelements/boundaryconditions/ggibcbase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
#include "openfoam/caseelements/boundaryconditions/emptybc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"
#include "openfoam/caseelements/boundaryconditions/mixingplaneggibc.h"
#include "openfoam/caseelements/boundaryconditions/massflowbc.h"
#include "openfoam/caseelements/boundaryconditions/simplebc.h"
#include "openfoam/caseelements/boundaryconditions/cyclicpairbc.h"
#include "openfoam/caseelements/boundaryconditions/compressibleinletbc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/exptdatainletbc.h"
#include "openfoam/caseelements/boundaryconditions/suctioninletbc.h"
#include "openfoam/caseelements/turbulencemodel.h"
#include "openfoam/caseelements/basic/minimumtimesteplimit.h"
#include "openfoam/caseelements/basic/mirrormesh.h"
#include "openfoam/caseelements/basic/srfoption.h"
#include "openfoam/caseelements/basic/twophasetransportproperties.h"
#include "openfoam/caseelements/basic/copyfiles.h"
#include "openfoam/caseelements/basic/cavitationtwophasetransportproperties.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/basic/gravity.h"
#include "openfoam/caseelements/basic/transportmodel.h"
#include "openfoam/caseelements/basic/decomposepardict.h"
#include "openfoam/caseelements/basic/volumedrag.h"
#include "openfoam/caseelements/basic/constantpressuregradientsource.h"
#include "openfoam/caseelements/basic/porouszone.h"
#include "openfoam/caseelements/basic/customdictentries.h"
#include "openfoam/caseelements/basic/limitquantities.h"
#include "openfoam/caseelements/basic/setfieldsconfiguration.h"
#include "openfoam/caseelements/basic/cellsetoption_selection.h"
#include "openfoam/caseelements/basic/source.h"
#include "openfoam/caseelements/basic/passivescalar.h"
#include "openfoam/caseelements/basic/mrfzone.h"
#include "openfoam/caseelements/basic/fixedvalueconstraint.h"
#include "openfoam/caseelements/basic/pressuregradientsource.h"
#include "openfoam/caseelements/basic/wallheatflux.h"
#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/caseelements/analysiscaseelements.h"
#include "openfoam/caseelements/boundarycondition.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/fielddata.h"
#include "openfoam/cfmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofdicts.h"
#include "openfoam/setfields.h"
#include "openfoam/createpatch.h"

#include "code_aster/caexportfile.h"
#include "code_aster/codeasterrun.h"
#include "base/exception.h"

using namespace insight;
using namespace insight::bmd;
using namespace insight::multiphaseBC;
using namespace insight::createPatchOps;
%}

%include "common.i"
%include "exception.i"

%exception {
	try {
	$action
	}
        catch (const std::exception& e) {
                SWIG_exception(SWIG_RuntimeError, (std::string("Insight exception: ")+e.what()).c_str());
	}
}


%typemap(out) int& getInt %{
  $result = PyInt_FromLong(*$1);
%}
%typemap(out) bool& getBool %{
  $result = PyBool_FromLong(*$1);
%}
%typemap(out) double& getDouble %{
  $result = PyFloat_FromDouble(*$1);
%}


// %feature("director") Analysis;
%include "base/boost_include.h"
%include "base/factory.h"
%include "base/parameter.h"
%include "base/parameterset.h"
%include "base/resultelement.h"
%include "base/resultelementcollection.h"
%include "base/resultset.h"

%include "base/resultelements/numericalresult.h"
%include "base/resultelements/scalarresult.h"
%include "base/resultelements/vectorresult.h"
%include "base/resultelements/tabularresult.h"
%include "base/resultelements/resultsection.h"
%include "base/resultelements/polarchart.h"
%include "base/resultelements/image.h"
%include "base/resultelements/contourchart.h"
%include "base/resultelements/comment.h"
%include "base/resultelements/chart.h"
%include "base/resultelements/attributeresulttable.h"

%include "base/parameter.h"
%include "base/parameters/simpleparameter.h"
%include "base/parameters/doublerangeparameter.h"
%include "base/parameters/selectionparameter.h"
%include "base/parameters/matrixparameter.h"
%include "base/parameters/pathparameter.h"
%include "base/parameters/arrayparameter.h"
%include "base/parameterset.h"
%include "base/analysis.h"
%include "base/parameterstudy.h"
%include "base/case.h"
%include "base/caseelement.h"
%include "base/linearalgebra.h"
%include "base/tools.h"
%include "base/remotelocation.h"
%include "base/remoteexecution.h"

%include "openfoam/caseelements/boundarycondition.h"
%include "openfoam/openfoamcase.h"
%include "openfoam/openfoamdict.h"
%include "openfoam/openfoamtools.h"
%include "openfoam/solveroutputanalyzer.h"
%include "openfoam/caseelements/boundarycondition.h"
%include "openfoam/caseelements/analysiscaseelements.h"
%include "openfoam/paraview.h"
%include "openfoam/ofes.h"
%include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
%include "openfoam/caseelements/numerics/laplacianfoamnumerics.h"
%include "openfoam/caseelements/numerics/cavitatingfoamnumerics.h"
%include "openfoam/caseelements/numerics/reactingfoamnumerics.h"
%include "openfoam/caseelements/numerics/interphasechangefoamnumerics.h"
%include "openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h"
%include "openfoam/caseelements/numerics/meshingnumerics.h"
%include "openfoam/caseelements/numerics/potentialfreesurfacefoamnumerics.h"
%include "openfoam/caseelements/numerics/fsidisplacementextrapolationnumerics.h"
%include "openfoam/caseelements/numerics/pimplesettings.h"
%include "openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h"
%include "openfoam/caseelements/numerics/tetfemnumerics.h"
%include "openfoam/caseelements/numerics/steadycompressiblenumerics.h"
%include "openfoam/caseelements/numerics/unsteadycompressiblenumerics.h"
%include "openfoam/caseelements/numerics/reactingparcelfoamnumerics.h"
%include "openfoam/caseelements/numerics/fanumerics.h"
%include "openfoam/caseelements/numerics/simpledymfoamnumerics.h"
%include "openfoam/caseelements/numerics/magneticfoamnumerics.h"
%include "openfoam/caseelements/numerics/interfoamnumerics.h"
%include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
%include "openfoam/caseelements/numerics/potentialfoamnumerics.h"
%include "openfoam/caseelements/numerics/ltsinterfoamnumerics.h"
%include "openfoam/caseelements/thermophysicalcaseelements.h"
%include "openfoam/caseelements/turbulencemodels/smagorinsky_lesmodel.h"
%include "openfoam/caseelements/turbulencemodels/lrr_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/kepsilonbase_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/kepsilon_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/dynsmagorinsky_lesmodel.h"
%include "openfoam/caseelements/turbulencemodels/laminar_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/komegasst2_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/oneeqeddy_lesmodel.h"
%include "openfoam/caseelements/turbulencemodels/dynoneeqeddy_lesmodel.h"
%include "openfoam/caseelements/turbulencemodels/lemoshybrid_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/spalartallmaras_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/komegasst_lowre_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/komegasst_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/komegahe_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/realizablekepsilon_rasmodel.h"
%include "openfoam/caseelements/turbulencemodels/wale_lesmodel.h"
%include "openfoam/caseelements/electromagneticscaseelements.h"
%include "openfoam/caseelements/dynamicmesh/velocitytetfemmotionsolver.h"
%include "openfoam/caseelements/dynamicmesh/displacementfvmotionsolver.h"
%include "openfoam/caseelements/dynamicmesh/solidbodymotiondynamicmesh.h"
%include "openfoam/caseelements/dynamicmesh/rigidbodymotiondynamicmesh.h"
%include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"
%include "openfoam/caseelements/boundaryconditions/wallbc.h"
%include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
%include "openfoam/caseelements/boundaryconditions/ggibc.h"
%include "openfoam/caseelements/boundaryconditions/turbulentvelocityinletbc.h"
%include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"
%include "openfoam/caseelements/boundaryconditions/cyclicggibc.h"
%include "openfoam/caseelements/boundaryconditions/mappedvelocityinletbc.h"
%include "openfoam/caseelements/boundaryconditions/symmetrybc.h"
%include "openfoam/caseelements/boundaryconditions/potentialfreesurfacebc.h"
%include "openfoam/caseelements/boundaryconditions/overlapggibc.h"
%include "openfoam/caseelements/boundaryconditions/ggibcbase.h"
%include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
%include "openfoam/caseelements/boundaryconditions/emptybc.h"
%include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"
%include "openfoam/caseelements/boundaryconditions/mixingplaneggibc.h"
%include "openfoam/caseelements/boundaryconditions/massflowbc.h"
%include "openfoam/caseelements/boundaryconditions/simplebc.h"
%include "openfoam/caseelements/boundaryconditions/cyclicpairbc.h"
%include "openfoam/caseelements/boundaryconditions/compressibleinletbc.h"
%include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
%include "openfoam/caseelements/boundaryconditions/exptdatainletbc.h"
%include "openfoam/caseelements/boundaryconditions/suctioninletbc.h"
%include "openfoam/caseelements/basic/minimumtimesteplimit.h"
%include "openfoam/caseelements/basic/mirrormesh.h"
%include "openfoam/caseelements/basic/srfoption.h"
%include "openfoam/caseelements/basic/twophasetransportproperties.h"
%include "openfoam/caseelements/basic/copyfiles.h"
%include "openfoam/caseelements/basic/cavitationtwophasetransportproperties.h"
%include "openfoam/caseelements/basic/singlephasetransportmodel.h"
%include "openfoam/caseelements/basic/gravity.h"
%include "openfoam/caseelements/basic/decomposepardict.h"
%include "openfoam/caseelements/basic/volumedrag.h"
%include "openfoam/caseelements/basic/constantpressuregradientsource.h"
%include "openfoam/caseelements/basic/porouszone.h"
%include "openfoam/caseelements/basic/customdictentries.h"
%include "openfoam/caseelements/basic/limitquantities.h"
%include "openfoam/caseelements/basic/setfieldsconfiguration.h"
%include "openfoam/caseelements/basic/source.h"
%include "openfoam/caseelements/basic/passivescalar.h"
%include "openfoam/caseelements/basic/mrfzone.h"
%include "openfoam/caseelements/basic/fixedvalueconstraint.h"
%include "openfoam/caseelements/basic/pressuregradientsource.h"
%include "openfoam/caseelements/basic/wallheatflux.h"

%include "openfoam/openfoamanalysis.h"

%rename(Cylinder_Parameters) Cylinder::Parameters;

%include "openfoam/blockmesh_templates.h"

%include "code_aster/caexportfile.h"
%include "code_aster/codeasterrun.h"
