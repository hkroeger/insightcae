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

#include "base/analysis.h"
#include "base/parameterstudy.h"

namespace insight
{

    
    
    
class SimpleAnalysis
: public Analysis
{
public:
#include "simple_analysis__SimpleAnalysis__Parameters.h"
    
/* 
PARAMETERSET>>> SimpleAnalysis Parameters
x = double 0 "x value"
<<<PARAMETERSET
*/

public:
    declareType("SimpleAnalysis");
    SimpleAnalysis(const ParameterSet& ps, const boost::filesystem::path& exepath);
    static ParameterSet defaultParameters();
    ResultSetPtr operator()(ProgressDisplayer* p=NULL);
};




defineType(SimpleAnalysis);
addToFactoryTable(Analysis, SimpleAnalysis);
addToStaticFunctionTable(Analysis, SimpleAnalysis, defaultParameters);




SimpleAnalysis::SimpleAnalysis(const ParameterSet& ps, const boost::filesystem::path& exepath)
: Analysis("Test", "", ps, exepath)
{
}




ParameterSet SimpleAnalysis::defaultParameters()
{
    return Parameters::makeDefault();
}




ResultSetPtr SimpleAnalysis::operator()(ProgressDisplayer* pd)
{
    Parameters p(parameters());
    ResultSetPtr results( new ResultSet(parameters(), "Test", "") );
    
    results->insert("y", new ScalarResult( 
    
        3*pow(p.x,2), 
        
        "function value", "", ""));
    
    return results;
}



RangeParameterList rpl_SimpleAnalysis = list_of<std::string>("x");

class SimpleParameterStudy
: public ParameterStudy<SimpleAnalysis, rpl_SimpleAnalysis>
{
public:
    declareType("SimpleParameterStudy");
    
    SimpleParameterStudy(const ParameterSet& ps, const boost::filesystem::path& exepath)
    : ParameterStudy("Test", "", ps, exepath)
    {
    }
    
    
    virtual ResultSetPtr evaluateRuns()
    {
        ResultSetPtr results = ParameterStudy::evaluateRuns();

        std::string key="yTable";
        
        results->insert(key, table("", "", "x", 
                        list_of<std::string>("y")));
        
        const TabularResult& tab = 
            static_cast<const TabularResult&>(*(results->find(key)->second));
        
        arma::mat tabdat=tab.toMat();
        
        
        addPlot
        (
            results, executionPath(), "chartAirfoilCharacteristics",
            "$x$", "$y$",
            list_of<PlotCurve>
            (PlotCurve(arma::mat(join_rows(tabdat.col(0), tabdat.col(1))), 	"y_vs_x", "w lp t '$y=f(x)$"))
            ,
            "Function plot"
        );

        return results;
    }
};

defineType(SimpleParameterStudy);
addToFactoryTable(Analysis, SimpleParameterStudy);
addToStaticFunctionTable(Analysis, SimpleParameterStudy, defaultParameters);

}
