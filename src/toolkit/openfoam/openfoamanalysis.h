/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  Hannes Kroeger <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef INSIGHT_OPENFOAMANALYSIS_H
#define INSIGHT_OPENFOAMANALYSIS_H

#include "base/analysis.h"
#include "openfoam/openfoamcase.h"


namespace insight {
  
//class OpenFOAMCase;
  
void insertTurbulenceModel(OpenFOAMCase& cm, const std::string& name);

class OpenFOAMAnalysis
: public insight::Analysis
{
protected:
    bool stopFlag_;

public:
    OpenFOAMAnalysis(const std::string& name, const std::string& description);
    
    virtual void cancel();
    
    virtual insight::ParameterSet defaultParameters() const;

    virtual void createMesh(OpenFOAMCase& cm, const ParameterSet& p) =0;
    virtual void createCase(OpenFOAMCase& cm, const ParameterSet& p) =0;
    
    virtual void createDictsInMemory(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
    
    /**
     * Customize dictionaries before they get written to disk
     */
    virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
    
    virtual void writeDictsToDisk(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
    
    /**
     * Do modifications to the case when it has been created on disk
     */
    virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
    
    virtual void runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm, const ParameterSet& p);
    
    virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm, const ParameterSet& p);
    
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer);
};

}

#endif // INSIGHT_OPENFOAMANALYSIS_H
