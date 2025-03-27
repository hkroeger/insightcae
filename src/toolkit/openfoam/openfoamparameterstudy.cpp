
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

#include "openfoamparameterstudy.h"
#include "base/cppextensions.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"

namespace insight {
    

    
    
// defineType(OpenFOAMParameterStudy);




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
OpenFOAMParameterStudy<BaseAnalysis,var_params>::OpenFOAMParameterStudy
(
        const std::shared_ptr<supplementedInputDataBase>& sp,
        bool subcasesRemesh
)
: ParameterStudy<BaseAnalysis,var_params>(sp),
  subcasesRemesh_(subcasesRemesh)
{
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void OpenFOAMParameterStudy<BaseAnalysis,var_params>::modifyInstanceParameters(
    const std::string& subcase_name,
    ParameterSet& newp ) const
{
  boost::filesystem::path oldmf =
        newp.get<PathParameter>("run/mapFrom").filePath();
  boost::filesystem::path newmf = "";

  if (oldmf!="")
  {
    oldmf=boost::filesystem::absolute(oldmf);
    newmf = oldmf / subcase_name;
    if (!boost::filesystem::exists(newmf)) 
    {
      insight::Warning(
            "No matching subcase exists in %s for mapping of subcase %s! Omitting.",
            oldmf.string().c_str(), subcase_name.c_str() );
      newmf="";
    }
  }
  newp.get<PathParameter>("run/mapFrom").setOriginalFilePath(newmf);
}




template<
    class BaseAnalysis,
    const RangeParameterList& var_params
    >
ResultSetPtr OpenFOAMParameterStudy<BaseAnalysis,var_params>::operator()(
    ProgressDisplayer& displayer )
{
    auto& ps = this->parameters(); //Analysis::parameters_;

    // generate the mesh in the top level case first
    path dir = this->executionPath();
    //parameters_.saveToFile(dir/"parameters.ist", type());

    {
//     OpenFOAMAnalysis* base_case=static_cast<OpenFOAMAnalysis*>(baseAnalysis_.get());

        auto machine = ps.getString("run/machine");
        auto OFEname = ps.getString("run/OFEname");

        OFEnvironment ofe = OFEs::get(OFEname);
        ofe.setExecutionMachine(machine);

        path exep=this->executionPath();

        // Generate a valid parameterset with actual values for mesh mesh genration
        // use first value from each range
        auto defp = ps.cloneParameterSet();
        for (int j=0; j<var_params.size(); j++)
        {
            // Replace RangeParameter by first actual single value
            const DoubleRangeParameter& rp = ps.template get<insight::DoubleRangeParameter>(var_params[j]);
            auto dp=rp.toDoubleParameter(rp.values().begin());
            defp->replace(var_params[j], std::move(dp));
        }
//     base_case->setParameters(defp);

//     base_case->setExecutionPath(exep);
        base_case_ = std::dynamic_unique_ptr_cast<OpenFOAMAnalysis>(
            Analysis::analyses()(BaseAnalysis::typeName,
                Analysis::supplementedInputDatas()(BaseAnalysis::typeName,
                    ParameterSetInput(*defp), exep, displayer)) );

        if (!base_case_)
        {
            throw insight::Exception("Internal Error: invalid base analysis type for OpenFOAMParameterStudy!");
        }

        dir = base_case_->executionPath();

        if (!subcasesRemesh_)
        {
            OpenFOAMCase meshCase(ofe);
            if (!meshCase.meshPresentOnDisk(dir))
                base_case_->createMesh(meshCase, displayer);
            else
                std::cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<std::endl;
        }
    }

    path old_lp=ps.template get<PathParameter>("mesh/linkmesh").originalFilePath();
    if (!subcasesRemesh_)
        ps.template get<PathParameter>("mesh/linkmesh").setOriginalFilePath(
            boost::filesystem::absolute(this->executionPath()) );
    this->setupQueue();
    ps.template get<PathParameter>("mesh/linkmesh").setOriginalFilePath( old_lp );

    this->processQueue(displayer);
    ResultSetPtr results = this->evaluateRuns();

    evaluateCombinedResults(results);

    return results;
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void OpenFOAMParameterStudy<BaseAnalysis,var_params>::evaluateCombinedResults(ResultSetPtr&)
{
}



    
}
