
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

namespace insight {
    

    
    
// defineType(OpenFOAMParameterStudy);




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
OpenFOAMParameterStudy<BaseAnalysis,var_params>::OpenFOAMParameterStudy
(
        const std::string& name, 
        const std::string& description, 
        const ParameterSet& ps,
        const boost::filesystem::path& exePath,
        bool subcasesRemesh
)
: ParameterStudy<BaseAnalysis,var_params>
  (
    name, description, ps, exePath
  ),
  subcasesRemesh_(subcasesRemesh)
{
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void OpenFOAMParameterStudy<BaseAnalysis,var_params>::modifyInstanceParameters(const std::string& subcase_name, ParameterSetPtr& newp) const
{
  boost::filesystem::path oldmf = newp->get<PathParameter>("run/mapFrom")();
  boost::filesystem::path newmf = "";
  if (oldmf!="")
  {
    oldmf=boost::filesystem::absolute(oldmf);
    newmf = oldmf / subcase_name;
    if (!boost::filesystem::exists(newmf)) 
    {
      insight::Warning("No matching subcase exists in "+oldmf.string()+" for mapping of subcase "+subcase_name+"! Omitting.");
      newmf="";
    }
  }
  newp->get<PathParameter>("run/mapFrom")() = newmf;
}




template<
    class BaseAnalysis,
    const RangeParameterList& var_params
    >
ResultSetPtr OpenFOAMParameterStudy<BaseAnalysis,var_params>::operator()(ProgressDisplayer* displayer)
{
    ParameterSet& p = this->parameters_; //Analysis::parameters_;
    // generate the mesh in the top level case first
    path dir = setupExecutionEnvironment();
    //parameters_.saveToFile(dir/"parameters.ist", type());

    {
//     OpenFOAMAnalysis* base_case=static_cast<OpenFOAMAnalysis*>(baseAnalysis_.get());

        PSSTR(p, "run", machine);
        PSSTR(p, "run", OFEname);

        OFEnvironment ofe = OFEs::get(OFEname);
        ofe.setExecutionMachine(machine);

        path exep=executionPath();

        // Generate a valid parameterset with actual values for mesh mesh genration
        // use first value from each range
        ParameterSet defp(p);
        for (int j=0; j<var_params.size(); j++)
        {
            // Replace RangeParameter by first actual single value
            const DoubleRangeParameter& rp = p.get<DoubleRangeParameter>(var_params[j]);
            DoubleParameter* dp=rp.toDoubleParameter(rp.values().begin());
            defp.replace(var_params[j], dp);
        }
//     base_case->setParameters(defp);

//     base_case->setExecutionPath(exep);
        base_case_.reset( dynamic_cast<OpenFOAMAnalysis*>(Analysis::lookup(BaseAnalysis::typeName, defp, exep)) );

        if (!base_case_)
        {
            throw insight::Exception("Internal Error: invalid base analysis type for OpenFOAMParameterStudy!");
        }

        dir = base_case_->setupExecutionEnvironment();

        if (!subcasesRemesh_)
        {
            OpenFOAMCase meshCase(ofe);
            if (!meshCase.meshPresentOnDisk(dir))
                base_case_->createMesh(meshCase);
            else
                std::cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<std::endl;
        }
    }

    path old_lp=p.get<PathParameter>("mesh/linkmesh")();
    if (!subcasesRemesh_)
        p.get<PathParameter>("mesh/linkmesh")() = boost::filesystem::absolute(executionPath());
    setupQueue();
    p.get<PathParameter>("mesh/linkmesh")() = old_lp;

    processQueue(displayer);
    ResultSetPtr results = evaluateRuns();

    evaluateCombinedResults(results);

    return results;
}




template<
  class BaseAnalysis,
  const RangeParameterList& var_params
>
void OpenFOAMParameterStudy<BaseAnalysis,var_params>::evaluateCombinedResults(ResultSetPtr& results)
{
}



    
}
