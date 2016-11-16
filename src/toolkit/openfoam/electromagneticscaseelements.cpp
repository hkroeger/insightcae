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

#include "openfoam/electromagneticscaseelements.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{

magnet::magnet(OpenFOAMCase& c, Parameters const& p)
: OpenFOAMCaseElement(c, "magnet_"+p.name()),
  p_(p)
{
}

void magnet::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  OFDictData::list& maglist = transportProperties.addListIfNonexistent("magnets");
  
  OFDictData::list magpars;
  magpars.push_back(p_.name());
  magpars.push_back(p_.permeability());
  magpars.push_back(p_.remanence());
  magpars.push_back(OFDictData::vector3(p_.orientation()));
  maglist.push_back(magpars);
}
 
void magnet::modifyCaseOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const
{
  setSet
  (
    cm, location,
    list_of
    ("cellSet "+p_.name()+" new zoneToCell "+p_.name())
    ("faceSet "+p_.name()+" new cellToFace "+p_.name()+" all")
    ("faceSet "+p_.name()+" delete boundaryToFace")
  );
  cm.executeCommand(location, "setsToZones", list_of("-noFlipMap"));
}
 
FarFieldBC::FarFieldBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}


void FarFieldBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
      
    if ( (field.first=="psi") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    
    else
    {
      if (!(
	  MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}


}
