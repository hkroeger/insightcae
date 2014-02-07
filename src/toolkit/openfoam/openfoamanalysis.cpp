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


#include "openfoamanalysis.h"
#include "basiccaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace boost;
using namespace boost::assign;

namespace insight
{

void insertTurbulenceModel(OpenFOAMCase& cm, const std::string& name)
{
  turbulenceModel* model = turbulenceModel::lookup(name, cm);
  
  if (!model) 
    throw insight::Exception("Unrecognized RASModel selection: "+name);
  
  cm.insert(model);
}


OpenFOAMAnalysis::OpenFOAMAnalysis(const std::string& name, const std::string& description)
: Analysis(name, description)
{
}

ParameterSet OpenFOAMAnalysis::defaultParameters() const
{

  return ParameterSet
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
      
      ("run", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("machine", 	new StringParameter("", "machine or queue, where the external commands are executed on"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Execution parameters"
      ))
         
      ("solver", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("endTime", 	new DoubleParameter(1000.0, "simulation time at which the solver should stop"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Solver parameters"
      ))

      .convert_to_container<ParameterSet::EntryList>()
  );
}

}

