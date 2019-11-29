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

#ifndef INSIGHT_ELECTROMAGNETICSCASEELEMENTS_H
#define INSIGHT_ELECTROMAGNETICSCASEELEMENTS_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/caseelements/boundarycondition.h"

namespace insight 
{

    
    
class magnet
: public OpenFOAMCaseElement
{
public:
#include "electromagneticscaseelements__magnet__Parameters.h"
/*
PARAMETERSET>>> magnet Parameters

name = string "magnet" "name of magnet"
permeability = double 0.2 "magnet permeability"
remanence = double 1000.0 "magnet remanence"
orientation = vector (1 0 0) "magnet orientation"

<<<PARAMETERSET
*/


protected:
  Parameters p_;
  
public:
  magnet(OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  void modifyCaseOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const override;
  
  static std::string category() { return "Magnetics"; }
};




class FarFieldBC
: public BoundaryCondition
{
public:
#include "electromagneticscaseelements__FarFieldBC__Parameters.h"
/*
PARAMETERSET>>> FarFieldBC Parameters

<<<PARAMETERSET
*/
  
protected:
  Parameters p_;
  
public:
  FarFieldBC
  (
    OpenFOAMCase& c,
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    ParameterSet const& p = Parameters::makeDefault()
  );
  void addIntoFieldDictionaries(OFdicts& dictionaries) const override;
};

}

#endif
