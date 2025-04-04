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

#include "openfoam/caseelements/electromagneticscaseelements.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

#include "base/boost_include.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{

magnet::magnet(OpenFOAMCase& c, ParameterSetInput ip)
: OpenFOAMCaseElement(c, /*"magnet_"+ps.getString("name"), */
                          ip.forward<Parameters>())
{
}

void magnet::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  OFDictData::list& maglist = transportProperties.getList("magnets");
  
  OFDictData::list magpars;
  magpars.push_back(p().name);
  magpars.push_back(p().permeability);
  magpars.push_back(p().remanence);
  magpars.push_back(OFDictData::vector3(p().orientation));
  maglist.push_back(magpars);
}
 
void magnet::modifyCaseOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const
{
  setSet
  (
    cm, location,
    {
     "cellSet "+p().name+" new zoneToCell "+p().name,
     "faceSet "+p().name+" new cellToFace "+p().name+" all",
     "faceSet "+p().name+" delete boundaryToFace"
    }
  );
  cm.executeCommand(location, "setsToZones", { "-noFlipMap" });
}
 
FarFieldBC::FarFieldBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  ParameterSetInput ip
)
: BoundaryCondition(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>())
{
}


void FarFieldBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  for (const FieldList::value_type& field: OFcase().fields())
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
