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


#include "boundaryconditioncaseelements.h"
#include "openfoam/basiccaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/format.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{

FieldData::FieldData(double uniformSteadyValue)
: p_(Parameters::makeDefault())
{
  Parameters::fielddata_uniform_type data;
  data.values.resize(1);
  data.values[0].time=0;
  data.values[0].value=arma::ones(1)*uniformSteadyValue;
  p_.fielddata=data;
}
  
FieldData::FieldData(const arma::mat& uniformSteadyValue)
: p_(Parameters::makeDefault())
{
  Parameters::fielddata_uniform_type data;
  data.values.resize(1);
  data.values[0].time=0;
  data.values[0].value=uniformSteadyValue;
  p_.fielddata=data;
}
  
FieldData::FieldData(const ParameterSet& p)
: p_(p)
{
}


OFDictData::data FieldData::sourceEntry() const
{
  std::ostringstream os;
  
  if (const Parameters::fielddata_uniform_type *fd = boost::get<Parameters::fielddata_uniform_type>(&p_.fielddata) ) //(type=="uniform")
  {
    os<<" uniform unsteady";
    
    BOOST_FOREACH(const Parameters::fielddata_uniform_type::values_default_type& inst, fd->values)
    {
      os << " " << inst.time << " " << OFDictData::to_OF(inst.value);
    }
  }
  
  else if 
    (const Parameters::fielddata_linearProfile_type *fd = boost::get<Parameters::fielddata_linearProfile_type>(&p_.fielddata) )
  {
    os<<" linearProfile "
      <<OFDictData::to_OF(fd->p0)
      <<" "
      <<OFDictData::to_OF(fd->ep)
      <<" "
      <<OFDictData::to_OF(fd->ex)
      <<" "
      <<OFDictData::to_OF(fd->ez);

    os<<" (";
    BOOST_FOREACH(const Parameters::fielddata_linearProfile_type::cmap_default_type& cm, fd->cmap)
    {
      os<<" "<<cm.column<<" "<<cm.component;
    }
    os<<")";
    
    os<<" "
      <<"unsteady";
    
    BOOST_FOREACH(const Parameters::fielddata_linearProfile_type::values_default_type& inst, fd->values)
    {
      os << " " << inst.time << " " << inst.profile;
    }
  }

  else if 
    (const Parameters::fielddata_radialProfile_type *fd = boost::get<Parameters::fielddata_radialProfile_type>(&p_.fielddata) )
  {
    os<<" radialProfile "
      <<OFDictData::to_OF(fd->p0)
      <<" "
      <<OFDictData::to_OF(fd->ep)
      <<" "
      <<OFDictData::to_OF(fd->ex)
      <<" "
      <<OFDictData::to_OF(fd->ez);

    os<<" (";
    BOOST_FOREACH(const Parameters::fielddata_radialProfile_type::cmap_default_type& cm, fd->cmap)
    {
      os<<" "<<cm.column<<" "<<cm.component;
    }
    os<<")";
    
    os<<" "
      <<"unsteady";
    
    BOOST_FOREACH(const Parameters::fielddata_radialProfile_type::values_default_type& inst, fd->values)
    {
      os << " " << inst.time << " " << inst.profile;
    }
  }

  else if 
    (const Parameters::fielddata_fittedProfile_type *fd = boost::get<Parameters::fielddata_fittedProfile_type>(&p_.fielddata) )
  {
    os<<" fittedProfile "
      <<OFDictData::to_OF(fd->p0)
      <<" "
      <<OFDictData::to_OF(fd->ep)
      <<" "
      <<OFDictData::to_OF(fd->ex)
      <<" "
      <<OFDictData::to_OF(fd->ez)
      <<" "
      <<"unsteady";
    
    BOOST_FOREACH(const Parameters::fielddata_fittedProfile_type::values_default_type& inst, fd->values)
    {
      os << " " << inst.time;

      BOOST_FOREACH
      (
	const Parameters::fielddata_fittedProfile_type::values_default_type::component_coeffs_default_type& coeffs, 
	inst.component_coeffs
      )
      {
	os << " [";
	for (size_t cc=0; cc<coeffs.n_elem; cc++)
	  os<<" "<< str( format("%g") % coeffs[cc] );
	os<<" ]";
      }
    }
  }
  else
  {
    throw insight::Exception("Unknown field data description type!");
  }
  
  return os.str();
}

void FieldData::setDirichletBC(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data("extendedFixedValue");
  BC["source"]=sourceEntry();
}



double FieldData::representativeValueMag() const
{
  if (const Parameters::fielddata_uniform_type *fd = boost::get<Parameters::fielddata_uniform_type>(&p_.fielddata) )
  {
    double meanv=0.0;
    int s=0;
    BOOST_FOREACH(const Parameters::fielddata_uniform_type::values_default_type& inst, fd->values)
    {
      meanv+=pow(norm(inst.value, 2), 2);
      s++;
    }
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    meanv/=double(s);
    return sqrt(meanv);
  } 
  else if (const Parameters::fielddata_linearProfile_type *fd = boost::get<Parameters::fielddata_linearProfile_type>(&p_.fielddata) )
  {
    double avg=0.0;
    int s=0;
    BOOST_FOREACH(const Parameters::fielddata_linearProfile_type::values_default_type& inst, fd->values)
    {
      arma::mat xy;
      xy.load(inst.profile.c_str(), arma::raw_ascii);
      arma::mat I=integrate(xy);
      double avg_inst=0.0;
      BOOST_FOREACH(const Parameters::fielddata_linearProfile_type::cmap_default_type& cm, fd->cmap)
      {
	avg_inst+=pow(I(cm.column),2);
      }
      avg+=avg_inst;
      s++;
    }    
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    avg/=double(s);
    return sqrt(avg);
  }
  else
  {
    throw insight::Exception("not yet implemented!");
    return 0.0;
  }
}

double FieldData::maxValueMag() const
{
  double maxv=-DBL_MAX;
  if (const Parameters::fielddata_uniform_type *fd = boost::get<Parameters::fielddata_uniform_type>(&p_.fielddata) )
  {
    BOOST_FOREACH(const Parameters::fielddata_uniform_type::values_default_type& inst, fd->values)
    {
      maxv=std::max(maxv, norm(inst.value, 2));
    }
  } 
  else if (const Parameters::fielddata_linearProfile_type *fd = boost::get<Parameters::fielddata_linearProfile_type>(&p_.fielddata) )
  {
    BOOST_FOREACH(const Parameters::fielddata_linearProfile_type::values_default_type& inst, fd->values)
    {
      arma::mat xy;
      xy.load(inst.profile.c_str(), arma::raw_ascii);
      arma::mat mag_inst(arma::zeros(xy.n_rows));
      int i=0;
      BOOST_FOREACH(const Parameters::fielddata_linearProfile_type::cmap_default_type& cm, fd->cmap)
      {
	mag_inst(i++) += pow(xy(i, 1+cm.column),2);
      }
      maxv=std::max(maxv, as_scalar(arma::max(sqrt(mag_inst))));
    }    
  }
  else
  {
    throw insight::Exception("not yet implemented!");
    return 0.0;
  }
  return maxv;
}



Parameter* FieldData::defaultParameter(const arma::mat& reasonable_value, const std::string& description)
{
  return Parameters::makeDefault().get<SubsetParameter>("fielddata").clone();
}


BoundaryCondition::BoundaryCondition(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"BC"),
  patchName_(patchName),
  type_(boundaryDict.subDict(patchName).getString("type")),
  nFaces_(boundaryDict.subDict(patchName).getInt("nFaces")),
  startFace_(boundaryDict.subDict(patchName).getInt("startFace"))  
{
}

void BoundaryCondition::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["type"]=type_;
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
}

void BoundaryCondition::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent("boundaryField");
    OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_);
  }
}

void BoundaryCondition::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.addListIfNonexistent("libs").insertNoDuplicate( OFDictData::data("\"libextendedFixedValueBC.so\"") );

  addIntoFieldDictionaries(dictionaries);
  
  OFDictData::dict bndsubd;
  addOptionsToBoundaryDict(bndsubd);
  insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
}

void BoundaryCondition::insertIntoBoundaryDict
(
  OFdicts& dictionaries, 
  const string& patchName,
  const OFDictData::dict& bndsubd
)
{
  // contents is created as list of string / subdict pairs
  // patches have to appear ordered by "startFace"!
  OFDictData::dict& boundaryDict=dictionaries.addDictionaryIfNonexistent("constant/polyMesh/boundary");
  if (boundaryDict.size()==0)
    boundaryDict.addListIfNonexistent("");
  
  OFDictData::list& bl=
    *boost::get<OFDictData::list>( &boundaryDict.begin()->second );
  
  //std::cout<<"Configuring "<<patchName_<<std::endl;
  // search, if patchname is already present; replace, if yes
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (std::string *name = boost::get<std::string>(&(*i)))
    {
      //cout<<"found "<<*name<<endl;
      if ( *name == patchName )
      {
	i++;
	*i=bndsubd;
	return;
      }
    }
  }
  
  // not found, insert (at the right location)
  OFDictData::list::iterator j = bl.end();
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (OFDictData::dict *d = boost::get<OFDictData::dict>(&(*i)))
    {
      if (d->getInt("startFace") > bndsubd.getInt("startFace") ) 
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
      // patch with 0 faces has to be inserted before the face with the same start address but nonzero size
      if ( (d->getInt("startFace") == bndsubd.getInt("startFace") ) && (bndsubd.getInt("nFaces") == 0) )
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
    }
  }
  j = bl.insert( j, OFDictData::data(patchName) );
  bl.insert( j+1, bndsubd );
  
  OFDictData::dict::iterator oe=boundaryDict.begin();
  std::swap( boundaryDict[lexical_cast<std::string>(bl.size()/2)], oe->second );
  boundaryDict.erase(oe);
}

bool BoundaryCondition::providesBCsForPatch(const std::string& patchName) const
{
  return (patchName == patchName_);
}


SimpleBC::SimpleBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const std::string className)
: BoundaryCondition(c, patchName, boundaryDict),
  className_(className)
{
  if ( (OFversion()>=230) && (className_=="symmetryPlane")) className_="symmetry";
  type_=className_;
}

void SimpleBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (className_=="cyclic") && ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      std::string tname=className_;
      BC["type"]=OFDictData::data(tname);
    }
  }
}


CyclicPairBC::CyclicPairBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"CyclicBC"),
  patchName_(patchName)
{
  if (c.OFversion()>=210)
  {
    nFaces_=boundaryDict.subDict(patchName_+"_half0").getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_+"_half0").getInt("startFace");
    nFaces1_=boundaryDict.subDict(patchName_+"_half1").getInt("nFaces");
    startFace1_=boundaryDict.subDict(patchName_+"_half1").getInt("startFace");
  }
  else
  {
    nFaces_=boundaryDict.subDict(patchName_).getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_).getInt("startFace");
  }
}

void CyclicPairBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  addIntoFieldDictionaries(dictionaries);
  
  OFDictData::dict bndsubd, bndsubd1;
  bndsubd["type"]="cyclic";
  bndsubd["nFaces"]=nFaces_;
  bndsubd["startFace"]=startFace_;
  bndsubd1["type"]="cyclic";
  bndsubd1["nFaces"]=nFaces1_;
  bndsubd1["startFace"]=startFace1_;
  
  if (OFversion()>=210)
  {
    bndsubd["neighbourPatch"]=patchName_+"_half1";
    bndsubd1["neighbourPatch"]=patchName_+"_half0";
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half0", bndsubd);
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half1", bndsubd1);
  }
  else
  {
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
  }
}

void CyclicPairBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent("boundaryField");
    
    if (OFversion()>=210)
    {
      OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_+"_half0");
      OFDictData::dict& BC1=boundaryField.addSubDictIfNonexistent(patchName_+"_half1");
      
      if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      {
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC1);
      }
      else
      {
	BC["type"]="cyclic";
	BC1["type"]="cyclic";
      }
    }
    else
    {
      OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_);
      
      if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      {
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
      }
      else
      {
	BC["type"]="cyclic";
      }
    }
  }
}

bool CyclicPairBC::providesBCsForPatch(const std::string& patchName) const
{
  if (OFversion()>=210)
    return ( (patchName == patchName_+"_half0") || (patchName == patchName_+"_half1") );
  else
    return ( patchName == patchName_ );
}



GGIBCBase::GGIBCBase(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void GGIBCBase::modifyMeshOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const
{
  if (OFversion()<170)
  {
    setSet
    (
      cm, location,
      list_of<std::string>    
      ("faceSet "+p_.zone()+" new patchToFace "+patchName_)
    );
    cm.executeCommand(location, "setsToZones", list_of<std::string>("-noFlipMap") );
  }
}


GGIBC::GGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: GGIBCBase(c, patchName, boundaryDict, p),
  p_(p)
{
}

void GGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch();
    bndDict["matchTolerance"]= 0.001;
    bndDict["lowWeightCorrection"]=0.1;
    //bndDict["transform"]= "rotational";    
  }
  else
  {
    bndDict["type"]="ggi";
    bndDict["shadowPatch"]= p_.shadowPatch();
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
    bndDict["bridgeOverlap"]=p_.bridgeOverlap();
    bndDict["zone"]=p_.zone();
  }
}

void GGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("ggi");
    }
  }
}


CyclicGGIBC::CyclicGGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: GGIBCBase(c, patchName, boundaryDict, p),
  p_(p)
{
}

void CyclicGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch();
    bndDict["matchTolerance"]= 0.001;
    bndDict["transform"]= "rotational";    
    bndDict["rotationCentre"]=OFDictData::vector3(p_.rotationCentre());
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis());
    bndDict["rotationAngle"]=p_.rotationAngle();
    bndDict["lowWeightCorrection"]=0.1;
  }
  else
  {
    bndDict["type"]="cyclicGgi";
    bndDict["shadowPatch"]= p_.shadowPatch();
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
    bndDict["bridgeOverlap"]=p_.bridgeOverlap();
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis());
    bndDict["rotationAngle"]=p_.rotationAngle();
    bndDict["zone"]=p_.zone();
  }
}

void CyclicGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("cyclicGgi");
    }
  }
}




OverlapGGIBC::OverlapGGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: GGIBCBase(c, patchName, boundaryDict, p),
  p_(p)
{
}

void OverlapGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  
  bndDict["type"]="overlapGgi";
  bndDict["shadowPatch"]= p_.shadowPatch();
  bndDict["bridgeOverlap"]=p_.bridgeOverlap();
  bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
  bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis());
  bndDict["nCopies"]=p_.nCopies();
  bndDict["zone"]=p_.zone();
}

void OverlapGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      BC["type"]=OFDictData::data("overlapGgi");
    }
  }
}



MixingPlaneGGIBC::MixingPlaneGGIBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  Parameters const &p 
) : GGIBCBase(c, patchName, boundaryDict, p),
  p_(p)
{
  if ((OFversion()<160) || (OFversion()>=170))
  {
    throw insight::Exception("MixingPlane interface is not available in the selected OF version!");
  }
}

void MixingPlaneGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  bndDict["type"]="mixingPlane";
  bndDict["shadowPatch"]= p_.shadowPatch();
  bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
  bndDict["bridgeOverlap"]=p_.bridgeOverlap();
  bndDict["zone"]=p_.zone();
  
  OFDictData::dict csDict;
  csDict["type"]="cylindrical";
  csDict["name"]="mixingCS";
  csDict["origin"]=OFDictData::vector3(vec3(0,0,0));
  csDict["e1"]=OFDictData::vector3(vec3(1,0,0)); //radial
  csDict["e3"]=OFDictData::vector3(vec3(0,0,1)); // rot axis
  csDict["inDegrees"]=true;
  bndDict["coordinateSystem"]=csDict;

  OFDictData::dict rpDict;
  rpDict["sweepAxis"]="Theta";
  rpDict["stackAxis"]="R"; // axial interface
  rpDict["discretisation"]="bothPatches";
  bndDict["ribbonPatch"]=rpDict;

}

void MixingPlaneGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      BC["type"]=OFDictData::data("mixingPlane");
    }
  }
}


void MixingPlaneGGIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoDictionaries(dictionaries);

  OFDictData::dict& fvSchemes=dictionaries.addDictionaryIfNonexistent("system/fvSchemes");
  
  OFDictData::dict mpd;
  mpd["default"]="areaAveraging";
  mpd["p"]="areaAveraging";
  mpd["U"]="areaAveraging";
  mpd["k"]="fluxAveraging";
  mpd["epsilon"]="fluxAveraging";
  mpd["omega"]="fluxAveraging";
  mpd["nuTilda"]="fluxAveraging";

  if (fvSchemes.find("mixingPlane")==fvSchemes.end())
    fvSchemes["mixingPlane"]=mpd;
  
#warning There is probably a better location for this setup modification
  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  fvSolution.subDict("solvers")["p"]=stdSymmSolverSetup(1e-7, 0.01);

}



namespace multiphaseBC
{
  
multiphaseBC::~multiphaseBC()
{
}

void multiphaseBC::addIntoDictionaries(OFdicts& dictionaries) const
{
}

uniformPhases::uniformPhases()
{}

uniformPhases::uniformPhases(const uniformPhases& o)
: phaseFractions_(o.phaseFractions_)
{}

uniformPhases::uniformPhases( const PhaseFractionList& p0 )
: phaseFractions_(p0)
{}

uniformPhases& uniformPhases::set(const std::string& name, double val)
{
  phaseFractions_[name]=val;
  return *this;
}

bool uniformPhases::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  const PhaseFractionList& f = phaseFractions_;
  if 
  (     
    (f.find(fieldname)!=f.end())
    && 
    (get<0>(fieldinfo)==scalarField) 
  )
  {
    std::ostringstream entry;
    entry << "uniform "<<f.find(fieldname)->second;
//     BC["type"]="fixedValue";
//     BC["value"]=entry.str();
    BC["type"]="inletOutlet";
    BC["inletValue"]=entry.str();
    BC["value"]=entry.str();
    return true;
  }
  else 
    return false;
}

}

SuctionInletBC::SuctionInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
 type_="patch";
}

void SuctionInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  p_.phasefractions()->addIntoDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("pressureInletOutletVelocity");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      (field.first=="T") 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform "+lexical_cast<string>(p_.T());
    }
    else if ( 
      ( (field.first=="p") || (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("totalPressure");
      BC["p0"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
      BC["U"]=OFDictData::data(p_.UName());
      BC["phi"]=OFDictData::data(p_.phiName());
      BC["rho"]=OFDictData::data(p_.rhoName());
      BC["psi"]=OFDictData::data(p_.psiName());
      BC["gamma"]=OFDictData::data(p_.gamma());
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if 
    ( 
      (
	(field.first=="k") ||
	(field.first=="epsilon") ||
	(field.first=="omega") ||
	(field.first=="nut") ||
	(field.first=="nuSgs") ||
	(field.first=="nuTilda")
      )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
      {
	BC["type"]=OFDictData::data("zeroGradient");
      }
    }
  }
}

MassflowBC::MassflowBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
 type_="patch";
}

void MassflowBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  p_.phasefractions()->addIntoDictionaries(dictionaries);
  
  double velocity=1.0; // required for turbulence quantities. No better idea yet...
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("flowRateInletVelocity");
      BC["rho"]=p_.rhoName();
      BC["massFlowRate"]=p_.massflow();
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      (field.first=="T") 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform "+lexical_cast<string>(p_.T());
    }
    else if ( 
      ( (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      if (OFversion()>=210)
	BC["type"]=OFDictData::data("fixedFluxPressure");
      else
	BC["type"]=OFDictData::data("buoyantPressure");
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_k( BC, velocity );
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_omega( BC, velocity );
    }
    else if ( (field.first=="epsilon") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_epsilon( BC, velocity );
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("calculated");
      BC["value"]="uniform "+lexical_cast<string>(1e-10);
    }
    else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_nuTilda( BC, velocity );      
    }
    else if ( (field.first=="R") && (get<0>(field.second)==symmTensorField) )
    {
      p_.turbulence().setDirichletBC_R( BC, velocity );
    }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
      {
	BC["type"]=OFDictData::data("zeroGradient");
      }
    }
  }
}

TurbulenceSpecification::TurbulenceSpecification(const uniformIntensityAndLengthScale& uil)
: boost::variant<
    uniformIntensityAndLengthScale,
    oneEqn,
    twoEqn
    >(uil)
{}

TurbulenceSpecification::TurbulenceSpecification(const oneEqn& oe)
: boost::variant<
    uniformIntensityAndLengthScale,
    oneEqn,
    twoEqn
    >(oe)
{}


TurbulenceSpecification::TurbulenceSpecification(const twoEqn& te)
: boost::variant<
    uniformIntensityAndLengthScale,
    oneEqn,
    twoEqn
    >(te)
{}



void TurbulenceSpecification::setDirichletBC_k(OFDictData::dict& BC, double U) const
{
  if (const uniformIntensityAndLengthScale* uil = get<uniformIntensityAndLengthScale>(this))
  {
    double I=get<0>(*uil), L=get<1>(*uil);
    
    double uprime=I*U;
    double k=max(1e-6, 3.*pow(uprime, 2)/2.);
//     BC["type"]="fixedValue";
//     BC["value"]="uniform "+lexical_cast<string>(k);
    BC["type"]="inletOutlet";
    BC["inletValue"]="uniform "+lexical_cast<string>(k);
    BC["value"]="uniform "+lexical_cast<string>(k);
  }
  else
  {
    throw insight::Exception("Unsupported kind of turbulence specification for selected turbulence model: BC for k cannot be deduced.");
  }
}

void TurbulenceSpecification::setDirichletBC_omega(OFDictData::dict& BC, double U) const
{
  if (const uniformIntensityAndLengthScale* uil = get<uniformIntensityAndLengthScale>(this))
  {
    double I=get<0>(*uil), L=get<1>(*uil);
    
    double uprime=I*U;
    double k=max(1e-6, 3.*pow(uprime, 2)/2.);
    double omega=sqrt(k)/L;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform "+lexical_cast<string>(omega);
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform "+lexical_cast<string>(omega);
    BC["value"]="uniform "+lexical_cast<string>(omega);
  }
  else
  {
    throw insight::Exception("Unsupported kind of turbulence specification for selected turbulence model: BC for omega cannot be deduced.");
  }
}

void TurbulenceSpecification::setDirichletBC_epsilon(OFDictData::dict& BC, double U) const
{
  if (const uniformIntensityAndLengthScale* uil = get<uniformIntensityAndLengthScale>(this))
  {
    double I=get<0>(*uil), L=get<1>(*uil);
    
    double uprime=I*U;
    double k=3.*pow(uprime, 2)/2.;
    double epsilon=0.09*pow(k, 1.5)/L;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform "+lexical_cast<string>(epsilon);
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform "+lexical_cast<string>(epsilon);
    BC["value"]="uniform "+lexical_cast<string>(epsilon);
  }
  else
  {
    throw insight::Exception("Unsupported kind of turbulence specification for selected turbulence model: BC for epsilon cannot be deduced.");
  }
}


void TurbulenceSpecification::setDirichletBC_nuTilda(OFDictData::dict& BC, double U) const
{
  if (const uniformIntensityAndLengthScale* uil = get<uniformIntensityAndLengthScale>(this))
  {
    double I=get<0>(*uil), L=get<1>(*uil);
    
    double nutilda=sqrt(1.5)*I * U * L;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform "+lexical_cast<string>(nutilda);
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform "+lexical_cast<string>(nutilda);
    BC["value"]="uniform "+lexical_cast<string>(nutilda);
  }
  else
  {
    throw insight::Exception("Unsupported kind of turbulence specification for selected turbulence model: BC for epsilon cannot be deduced.");
  }
}

void TurbulenceSpecification::setDirichletBC_R(OFDictData::dict& BC, double U) const
{
  if (const uniformIntensityAndLengthScale* uil = get<uniformIntensityAndLengthScale>(this))
  {
    double I=get<0>(*uil), L=get<1>(*uil);
    
    double uprime=I*U;
//     BC["type"]=OFDictData::data("fixedValue");
//     BC["value"]="uniform ("+lexical_cast<string>(uprime/3.)+" 0 0 "+lexical_cast<string>(uprime/3.)+" 0 "+lexical_cast<string>(uprime/3.)+")";
    BC["type"]=OFDictData::data("inletOutlet");
    BC["inletValue"]="uniform ("+lexical_cast<string>(uprime/3.)+" 0 0 "+lexical_cast<string>(uprime/3.)+" 0 "+lexical_cast<string>(uprime/3.)+")";
    BC["value"]="uniform ("+lexical_cast<string>(uprime/3.)+" 0 0 "+lexical_cast<string>(uprime/3.)+" 0 "+lexical_cast<string>(uprime/3.)+")";
  }
  else
  {
    throw insight::Exception("Unsupported kind of turbulence specification for selected turbulence model: BC for R cannot be deduced.");
  }
}




VelocityInletBC::VelocityInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  Parameters const& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
 type_="patch";
}


void VelocityInletBC::setField_p(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data("zeroGradient");
}

void VelocityInletBC::setField_U(OFDictData::dict& BC) const
{
  p_.velocity().setDirichletBC(BC);
}

void VelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  p_.phasefractions()->addIntoDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      setField_U(BC);
    }
    
    else if ( 
      (field.first=="p") && (get<0>(field.second)==scalarField) 
    )
    {
      setField_p(BC);
    }
    else if ( 
      (field.first=="T") 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      p_.T().setDirichletBC(BC);
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]="uniform "+lexical_cast<string>(p_.T());
    }    
    else if ( 
      ( (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      if (OFversion()>=210)
	BC["type"]=OFDictData::data("fixedFluxPressure");
      else
	BC["type"]=OFDictData::data("buoyantPressure");
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
    }
    
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
      p_.rho().setDirichletBC(BC);
    }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_k( BC, p_.velocity().representativeValueMag() );
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_omega( BC, p_.velocity().representativeValueMag() );
    }
    else if ( (field.first=="epsilon") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_epsilon( BC, p_.velocity().representativeValueMag() );
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("calculated");
      BC["value"]="uniform "+lexical_cast<string>(1e-10);
    }
    else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
    {
      p_.turbulence().setDirichletBC_nuTilda( BC, p_.velocity().representativeValueMag() );      
    }
    else if ( (field.first=="R") && (get<0>(field.second)==symmTensorField) )
    {
      p_.turbulence().setDirichletBC_R( BC, p_.velocity().representativeValueMag() );
    }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}


ExptDataInletBC::ExptDataInletBC
(
  OpenFOAMCase& c, 
  const string& patchName, 
  const OFDictData::dict& boundaryDict,
  const ExptDataInletBC::Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
 type_="patch";
}

void ExptDataInletBC::addDataDict(OFdicts& dictionaries, const std::string& prefix, const std::string& fieldname, const arma::mat& data) const
{
  OFDictData::dictFile& Udict=dictionaries.addDictionaryIfNonexistent(prefix+"/0/"+fieldname);
  Udict.isSequential=true;
  
  if (data.n_cols==1)
    Udict["a"]=0.0;
  else if (data.n_cols==3)
    Udict["a"]=OFDictData::vector3(vec3(0,0,0));
  else
    throw insight::Exception("Unhandled number of components: "+lexical_cast<std::string>(data.n_cols));
  
  OFDictData::list vals;
  for (int r=0; r<data.n_rows; r++)
  {
    if (data.n_cols==1)
      vals.push_back(data(r));
    else if (data.n_cols==3)
      vals.push_back(OFDictData::vector3(data.row(r).t()));
  }
  Udict["b"]=vals;
}

void ExptDataInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  p_.phasefractions()->addIntoDictionaries(dictionaries);
  
  std::string prefix="constant/boundaryData/"+patchName_;
  
  OFDictData::dictFile& ptsdict=dictionaries.addDictionaryIfNonexistent(prefix+"/points");
  ptsdict.isSequential=true;
  OFDictData::list pts;
  const arma::mat& ptdat=p_.points();
  for (int r=0; r<ptdat.n_rows; r++)
    pts.push_back(OFDictData::vector3(ptdat.row(r).t()));
  ptsdict["a"]=pts;
  
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
      
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("timeVaryingMappedFixedValue");
      BC["offset"]=OFDictData::vector3(vec3(0,0,0));
      BC["setAverage"]=false;
      BC["perturb"]=1e-3;
      
//       OFDictData::dictFile& Udict=dictionaries.addDictionaryIfNonexistent(prefix+"/0/U");
//       Udict.isSequential=true;
//       Udict["a"]=OFDictData::vector3(vec3(0,0,0));
//       
//       OFDictData::list vals;
//       const arma::mat& Udat=p_.velocity();
//       cout<<Udat<<endl;
//       for (int r=0; r<Udat.n_rows; r++)
// 	vals.push_back(OFDictData::vector3(Udat.row(r).t()));
//       Udict["b"]=vals;
      addDataDict(dictionaries, prefix, "U", p_.velocity());
    }
    
    else if ( 
      (field.first=="p") && (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
//     else if ( 
//       (field.first=="T") 
//       && 
//       (get<0>(field.second)==scalarField) 
//     )
//     {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]="uniform "+lexical_cast<string>(p_.T());
//     }    
    else if ( 
      ( (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      if (OFversion()>=210)
	BC["type"]=OFDictData::data("fixedFluxPressure");
      else
	BC["type"]=OFDictData::data("buoyantPressure");
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
    }
    
//     else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
//     {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
//     }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("timeVaryingMappedFixedValue");
      BC["offset"]=0.0;
      BC["setAverage"]=false;
      BC["perturb"]=1e-3;
      addDataDict(dictionaries, prefix, "k", p_.TKE());
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("timeVaryingMappedFixedValue");
      BC["offset"]=0.0;
      BC["setAverage"]=false;
      BC["perturb"]=1e-3;
      addDataDict(dictionaries, prefix, "omega", p_.epsilon()/(0.09*p_.TKE()));
    }
    else if ( (field.first=="epsilon") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("timeVaryingMappedFixedValue");
      BC["offset"]=0.0;
      BC["setAverage"]=false;
      BC["perturb"]=1e-3;
      addDataDict(dictionaries, prefix, "epsilon", p_.epsilon());
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      double nutilda=1e-10; //sqrt(1.5)*p_.turbulenceIntensity() * arma::norm(p_.velocity(), 2) * p_.mixingLength();
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform "+lexical_cast<string>(nutilda);
    }
//     else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
//     {
//       BC["type"]=OFDictData::data("timeVaryingMappedFixedValue");
//       BC["offset"]=0.0;
//       BC["setAverage"]=false;
// //       addDataDict(dictionaries, prefix, "nuTilda", p_.epsilon());
//     }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}



CompressibleInletBC::CompressibleInletBC
(
  OpenFOAMCase& c, 
  const string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const CompressibleInletBC::Parameters& p)
: VelocityInletBC(c, patchName, boundaryDict, p),
  p_(p)
{
 type_="patch";
}

void CompressibleInletBC::setField_p(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data( "fixedValue" );
  BC["value"]=OFDictData::data( "uniform "+lexical_cast<std::string>(p_.pressure()) );
}


// TurbulentVelocityInletBC::inflowInitializer::~inflowInitializer()
// {
// }
// 
// void TurbulentVelocityInletBC::inflowInitializer::addToInitializerList
// (
//   OFDictData::dict& d, 
//   const std::string& patchName,
//   const arma::mat& Ubulk,
//   const ParameterSet& params
// ) const
// {
//   d["patchName"]=patchName;
//   
//   std::string MeanVelocityModel = params.get<SelectableSubsetParameter>("meanvelocity").selection();
//   std::string ReynoldsStressModel = params.get<SelectableSubsetParameter>("reystress").selection();
//   std::string LengthScaleModel = params.get<SelectableSubsetParameter>("lengthscale").selection();
//   const ParameterSet& MeanVelocityModelParams = params.get<SelectableSubsetParameter>("meanvelocity")();
//   const ParameterSet& LengthScaleModelParams = params.get<SelectableSubsetParameter>("lengthscale")();
//   const ParameterSet& ReynoldsStressModelParams = params.get<SelectableSubsetParameter>("reystress")();
//   
//   d["MeanVelocityModel"]=MeanVelocityModel;
//   OFDictData::dict cd;
//   if (MeanVelocityModel=="PowerLawMeanVelocity")
//   {
//     cd["n"]=MeanVelocityModelParams.getDouble("n");
//   }
//   else if (MeanVelocityModel=="TabulatedMeanVelocity")
//   {
//     cd["fileNameX"]="\""+MeanVelocityModelParams.getString("fileNameX")+"\"";
//     cd["fileNameY"]="\""+MeanVelocityModelParams.getString("fileNameY")+"\"";
//     cd["fileNameZ"]="\""+MeanVelocityModelParams.getString("fileNameZ")+"\"";
//   }
//   else if (MeanVelocityModel=="DNSMeanVelocity")
//   {
//     cd["datasetName"]="\""+MeanVelocityModelParams.getString("datasetName")+"\"";
//     cd["xCompName"]="\""+MeanVelocityModelParams.getString("xCompName")+"\"";
//     cd["yCompName"]="\""+MeanVelocityModelParams.getString("yCompName")+"\"";
//     cd["zCompName"]="\""+MeanVelocityModelParams.getString("zCompName")+"\"";
//   }
//   else throw insight::Exception("Unsupported MeanVelocityModel: "+MeanVelocityModel);
//   d[MeanVelocityModel+"Coeffs"]=cd;
//   
//   d["LengthScaleModel"]=LengthScaleModel;
//   OFDictData::dict lcd;
//   if (LengthScaleModel=="FittedIsotropicLengthScaleModel")
//   {
//     arma::mat coeff=LengthScaleModelParams.get<VectorParameter>("Lcoeff")();
//     lcd["c0"]=coeff(0);
//     lcd["c1"]=coeff(1);
//     lcd["c2"]=coeff(2);
//     lcd["c3"]=coeff(3);
//   }
//   else if (LengthScaleModel=="FittedAnisotropicLengthScaleModel")
//   {
//     arma::mat Llongcoeff=LengthScaleModelParams.get<VectorParameter>("Llongcoeff")();
//     arma::mat Lwallcoeff=LengthScaleModelParams.get<VectorParameter>("Lwallcoeff")();
//     arma::mat Llatcoeff=LengthScaleModelParams.get<VectorParameter>("Llatcoeff")();
//     OFDictData::dict csd;
//     csd["c0"]=Llongcoeff(0);
//     csd["c1"]=Llongcoeff(1);
//     csd["c2"]=Llongcoeff(2);
//     csd["c3"]=Llongcoeff(3);
//     lcd["x"]=csd;
//     csd["c0"]=Lwallcoeff(0);
//     csd["c1"]=Lwallcoeff(1);
//     csd["c2"]=Lwallcoeff(2);
//     csd["c3"]=Lwallcoeff(3);
//     lcd["y"]=csd;
//     csd["c0"]=Llatcoeff(0);
//     csd["c1"]=Llatcoeff(1);
//     csd["c2"]=Llatcoeff(2);
//     csd["c3"]=Llatcoeff(3);
//     lcd["z"]=csd;
//   }
//   else throw insight::Exception("Unsupported LengthScaleModel: "+LengthScaleModel);
//   d[LengthScaleModel+"Coeffs"]=lcd;
// 
//   d["ReynoldsStressModel"]=ReynoldsStressModel;
//   OFDictData::dict rcd;
//   if (ReynoldsStressModel=="DNSReynoldsStresses")
//   {
//     rcd["datasetName"]="\""+ReynoldsStressModelParams.getString("datasetName")+"\"";
//     rcd["xCompName"]="\""+ReynoldsStressModelParams.getString("xCompName")+"\"";
//     rcd["yCompName"]="\""+ReynoldsStressModelParams.getString("yCompName")+"\"";
//     rcd["zCompName"]="\""+ReynoldsStressModelParams.getString("zCompName")+"\"";
//   }
//   else if (ReynoldsStressModel=="TabulatedKReynoldsStresses")
//   {
//     rcd["fileName"]="\""+ReynoldsStressModelParams.getPath("fileName").string()+"\"";
//   }
//   else if (ReynoldsStressModel=="WallLayerReynoldsStresses")
//   {
//     //rcd["fileName"]="\""+ReynoldsStressModelParams.getPath("fileName").string()+"\"";
//   }
//   else throw insight::Exception("Unsupported ReynoldsStressModel: "+ReynoldsStressModel);
//   d[ReynoldsStressModel+"Coeffs"]=rcd;
//   
// }
// 
// ParameterSet TurbulentVelocityInletBC::inflowInitializer::defaultParameters()
// {
//   arma::mat Clong, Clat;
//   Clong << 0.78102065<<-0.30801496<<0.18299657<<3.73012118;
//   Clat << 0.84107675<<-0.63386837<<0.62172817<<0.7780003;  
//   
//   return ParameterSet
//   (
//       boost::assign::list_of<ParameterSet::SingleEntry>
//       (
// 	"inflow", new SubsetParameter
// 	(
// 	  ParameterSet( list_of<ParameterSet::SingleEntry>
// 	  // Mean velocity
// 	  (
// 	    "meanvelocity",
// 	    
// 	    new SelectableSubsetParameter
// 	    (
// 	      
// 	      "PowerLawMeanVelocity", 
// 	      list_of<SelectableSubsetParameter::SingleSubset>
// 	      (
// 		"PowerLawMeanVelocity", new ParameterSet
// 		(
// 		  list_of<ParameterSet::SingleEntry>
// 		  ("n", new DoubleParameter(7., "denominator of exponent 1/n of mean velocity power law"))
// 		  .convert_to_container<ParameterSet::EntryList>()
// 		)
// 	      )
// 	      (
// 		"TabulatedMeanVelocity", new ParameterSet
// 		(
// 		  list_of<ParameterSet::SingleEntry>
// 		  ("fileNameX", new PathParameter("umeanaxial_vs_yp.txt", "name of the ascii file containing the profile of mean axial velocity"))
// 		  ("fileNameY", new PathParameter("umeanwallnormal_vs_yp.txt", "name of the ascii file containing the profile of mean wall normal velocity"))
// 		  ("fileNameZ", new PathParameter("umeanspanwise_vs_yp.txt", "name of the ascii file containing the profile of mean spanwise velocity"))
// 		  .convert_to_container<ParameterSet::EntryList>()
// 		)
// 	      )
// 	      (
// 		"DNSMeanVelocity", new ParameterSet
// 		(
// 		  boost::assign::list_of<ParameterSet::SingleEntry>
// 		  ("datasetName", new StringParameter("MKM_Channel", "name of DNS dataset"))
// 		  ("xCompName", new StringParameter("590/umean_vs_yp", "Name of x-velocity profile in dataset"))
// 		  ("yCompName", new StringParameter("590/vmean_vs_yp", "Name of y-velocity profile in dataset"))
// 		  ("zCompName", new StringParameter("590/wmean_vs_yp", "Name of z-velocity profile in dataset"))
// 		  .convert_to_container<ParameterSet::EntryList>()
// 		)
// 	      )
// // 	      (
// // 		"TabulatedMeanVelocity", new ParameterSet
// // 		(
// // 		  boost::assign::list_of<ParameterSet::SingleEntry>
// // 		  ("tablefile", new PathParameter("meanvelocity.txt", "file with tabular data of mean velocity"))
// // 		  .convert_to_container<ParameterSet::EntryList>()
// // 		)
// // 	      )
// 	      .convert_to_container<SelectableSubsetParameter::SubsetList>(),
// 	     
// 	      "Definition of the mean inflow velocity"
// 	    )
// 	  )
// 
// 	  // RMS
// 	  (
// 	    "reystress",
// 	    
// 	    new SelectableSubsetParameter
// 	    (
// 	      
// 	      "WallLayerReynoldsStresses",  // default selection
// 	      list_of<SelectableSubsetParameter::SingleSubset>
// 	      (
// 		"WallLayerReynoldsStresses", new ParameterSet
// 		(
// 		  ParameterSet::EntryList()
// 		)
// 	      )
// 	      (
// 		"DNSReynoldsStresses", new ParameterSet
// 		(
// 		  boost::assign::list_of<ParameterSet::SingleEntry>
// 		  ("datasetName", new StringParameter("MKM_Channel", "name of the DNS dataset"))
// 		  ("xCompName", new StringParameter("590/Ruu_vs_yp", "Name of Rxx profile in dataset"))
// 		  ("yCompName", new StringParameter("590/Rvv_vs_yp", "Name of Ryy profile in dataset"))
// 		  ("zCompName", new StringParameter("590/Rww_vs_yp", "Name of Rzz profile in dataset"))
// 		  .convert_to_container<ParameterSet::EntryList>()
// 		)
// 	      )
// 	      (
// 		"TabulatedKReynoldsStresses", new ParameterSet
// 		(
// 		  boost::assign::list_of<ParameterSet::SingleEntry>
// 		  ("fileName", new PathParameter("Kp_vs_ydelta.txt", "name of the ascii file containing the profile of TKE"))
// 		  .convert_to_container<ParameterSet::EntryList>()
// 		)
// 	      )
// 	      .convert_to_container<SelectableSubsetParameter::SubsetList>(),
// 	     
// 	      "Definition of the reynolds stresses"
// 	    )
// 	  )
// 	  
// 	  // length scale
// 	  (
// 	    "lengthscale",
// 	    
// 	    new SelectableSubsetParameter
// 	    (
// 	      
// 	      "FittedIsotropicLengthScaleModel",  // default selection
// 	      list_of<SelectableSubsetParameter::SingleSubset>
// 	      (
// 		"FittedIsotropicLengthScaleModel", new ParameterSet
// 		(
// 		  boost::assign::list_of<ParameterSet::SingleEntry>
// 		  ("Lcoeff",	new VectorParameter(Clong, "Coefficients of isotropic length scale profile fit"))
// 		  .convert_to_container<ParameterSet::EntryList>()
// 		)
// 	      )
// 	      (
// 		"FittedAnisotropicLengthScaleModel", new ParameterSet
// 		(
// 		  boost::assign::list_of<ParameterSet::SingleEntry>
// 		  ("Llongcoeff",	new VectorParameter(Clong, "Coefficients of longitudinal length scale profile fit"))
// 		  ("Lwallcoeff",	new VectorParameter(Clat, "Coefficients of wall-normal length scale profile fit"))
// 		  ("Llatcoeff",		new VectorParameter(Clat, "Coefficients of lateral length scale profile fit"))
// 		  .convert_to_container<ParameterSet::EntryList>()
// 		)
// 	      )
// 	      .convert_to_container<SelectableSubsetParameter::SubsetList>(),
// 	     
// 	      "Definition of the length scale"
// 	    )
// 	  )
// 
// 	  .convert_to_container<ParameterSet::EntryList>()
// 	  ), 
// 	  "Definition of the inflow boundary condition"
// 	)
//       )
//       
//       .convert_to_container<ParameterSet::EntryList>()
//   );
// }
// 
// TurbulentVelocityInletBC::pipeInflowInitializer::pipeInflowInitializer()
// {
// }
// 
// std::string TurbulentVelocityInletBC::pipeInflowInitializer::type() const
// {
//   return "pipeFlow";
// }
// 
// void TurbulentVelocityInletBC::pipeInflowInitializer::addToInitializerList
// (
//   OFDictData::dict& d, 
//   const std::string& patchName,
//   const arma::mat& Ubulk,
//   const ParameterSet& params
// ) const
// {
//   d["Ubulk"]=arma::norm(Ubulk, 2);
//   inflowInitializer::addToInitializerList(d, patchName, Ubulk, params);
// }
// 
// TurbulentVelocityInletBC::channelInflowInitializer::channelInflowInitializer()
// {
// }
// 
// std::string TurbulentVelocityInletBC::channelInflowInitializer::type() const
// {
//   return "channelFlow";
// }
// 
// void TurbulentVelocityInletBC::channelInflowInitializer::addToInitializerList
// (
//   OFDictData::dict& d, 
//   const std::string& patchName,
//   const arma::mat& Ubulk,
//   const ParameterSet& params
// ) const
// {
//   d["Ubulk"]=arma::norm(Ubulk, 2);
//   d["vertical"]=OFDictData::to_OF(vec3(0,1,0));
//   inflowInitializer::addToInitializerList(d, patchName, Ubulk, params);
// }

TurbulentVelocityInletBC::TurbulentVelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  ParameterSet const& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
 type_="patch";
}

const std::vector<std::string> TurbulentVelocityInletBC::inflowGenerator_types = boost::assign::list_of
   ("inflowGenerator<hatSpot>")
   ("inflowGenerator<gaussianSpot>")
   ("inflowGenerator<decayingTurbulenceSpot>")
   ("inflowGenerator<decayingTurbulenceVorton>")
   ("inflowGenerator<anisotropicVorton>")
   ("inflowGenerator<anisotropicVorton2>")
   ("inflowGenerator<modalTurbulence>")
   .convert_to_container<std::vector<std::string> >();
   
void TurbulentVelocityInletBC::setField_U(OFDictData::dict& BC) const
{
  ParameterSet ps=Parameters::makeDefault();
  p_.set(ps);
  
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {
    FieldData(ps.getSubset("umean")).setDirichletBC(BC);
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    BC["type"]= inflowGenerator_types[tu->type];
    BC["Umean"]=FieldData(ps.getSubset("umean")).sourceEntry();
    BC["c"]=FieldData(tu->volexcess).sourceEntry();
    BC["uniformConvection"]=tu->uniformConvection;
    BC["R"]=FieldData(ps.get<SelectableSubsetParameter>("turbulence")().getSubset("R")).sourceEntry();
    BC["L"]=FieldData(ps.get<SelectableSubsetParameter>("turbulence")().getSubset("L")).sourceEntry();
    BC["value"]="uniform (0 0 0)";
  }
}

void TurbulentVelocityInletBC::setField_p(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data("zeroGradient");
}

void TurbulentVelocityInletBC::setField_k(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean") ).representativeValueMag();
    
    double uprime=tu->intensity*U;
    double k=max(1e-6, 3.*pow(uprime, 2)/2.);
    BC["type"]="fixedValue";
    BC["value"]="uniform "+lexical_cast<string>(k);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    // set some small sgs energy
    BC["type"]="fixedValue";
    BC["value"]="uniform 1e-5";
  }
}

void TurbulentVelocityInletBC::setField_omega(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean") ).representativeValueMag();
    
    double uprime = tu->intensity*U;
    double k = max(1e-6, 3.*pow(uprime, 2)/2.);
    double omega = sqrt(k) / tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+lexical_cast<string>(omega);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field omega while inflow generator was selected!");
  }
}

void TurbulentVelocityInletBC::setField_epsilon(OFDictData::dict& BC) const
{

  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean") ).representativeValueMag();
    
    double uprime = tu->intensity*U;
    double k=3.*pow(uprime, 2)/2.;
    double epsilon=0.09*pow(k, 1.5)/tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+lexical_cast<string>(epsilon);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field epsilon while inflow generator was selected!");
  }

}

void TurbulentVelocityInletBC::setField_nuTilda(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean") ).representativeValueMag();
    
    double uprime = tu->intensity*U;
    double nutilda=sqrt(1.5)* uprime * tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+lexical_cast<string>(nutilda);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field nuTilda while inflow generator was selected!");
  }

}

void TurbulentVelocityInletBC::setField_R(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean") ).representativeValueMag();
    
    double uprime=tu->intensity*U;
    double kBy3=max(1e-6, pow(uprime, 2)/2.);
    BC["type"]="fixedValue";
    BC["value"]="uniform ("+str(format("%g 0 0 %g 0 %g") % kBy3 % kBy3 % kBy3 )+")";
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field R while inflow generator was selected!");
  }
}


void TurbulentVelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  
  if (boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))  
    controlDict.addListIfNonexistent("libs").push_back( OFDictData::data("\"libinflowGeneratorBC.so\"") );

  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
//   p_.phasefractions()->addIntoDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      setField_U(BC);
    }
    
    else if ( 
      (field.first=="p") && (get<0>(field.second)==scalarField) 
    )
    {
      setField_p(BC);
    }
//     else if ( 
//       (field.first=="T") 
//       && 
//       (get<0>(field.second)==scalarField) 
//     )
//     {
//       p_.T().setDirichletBC(BC);
// //       BC["type"]=OFDictData::data("fixedValue");
// //       BC["value"]="uniform "+lexical_cast<string>(p_.T());
//     }    
//     else if ( 
//       ( (field.first=="pd") || (field.first=="p_rgh") )
//       && 
//       (get<0>(field.second)==scalarField) 
//     )
//     {
//       if (OFversion()>=210)
// 	BC["type"]=OFDictData::data("fixedFluxPressure");
//       else
// 	BC["type"]=OFDictData::data("buoyantPressure");
// //       BC["type"]=OFDictData::data("calculated");
// //       BC["value"]=OFDictData::data("uniform 0");
//     }
    
//     else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
//     {
// //       BC["type"]=OFDictData::data("fixedValue");
// //       BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
//       p_.rho().setDirichletBC(BC);
//     }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      setField_k(BC);
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      setField_omega(BC);
    }
    else if ( (field.first=="epsilon") && (get<0>(field.second)==scalarField) )
    {
      setField_epsilon(BC);
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("calculated");
      BC["value"]="uniform "+lexical_cast<string>(1e-10);
    }
    else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
    {
      setField_nuTilda(BC);
    }
    else if ( (field.first=="R") && (get<0>(field.second)==symmTensorField) )
    {
      setField_R(BC);
    }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]="zeroGradient";
//       BC["value"]="uniform 1e-10";
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
// 	  ||
// 	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}



// ParameterSet TurbulentVelocityInletBC::defaultParameters()
// {
//   return ParameterSet
//   (
//     boost::assign::list_of<ParameterSet::SingleEntry>
//     ("uniformConvection", new BoolParameter(false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
//     ("volexcess", new DoubleParameter(16.0, "Volumetric overlapping of spots"))
//     (
//       "type", new SelectionParameter(0,
// 	list_of<string>
// 	("inflowGenerator<hatSpot>")
// 	("inflowGenerator<gaussianSpot>")
// 	("inflowGenerator<decayingTurbulenceSpot>")
// 	("inflowGenerator<decayingTurbulenceVorton>")
// 	("inflowGenerator<anisotropicVorton>")
// 	("modalTurbulence")
//       ,
//       "Type of inflow generator")
//     )
//     ("L", FieldData::defaultParameter(vec3(1,1,1), "Origin of the prescribed integral length scale field"))
//     ("R", FieldData::defaultParameter(arma::zeros(6), "Origin of the prescribed reynolds stress field"))
//     .convert_to_container<ParameterSet::EntryList>()
//   );
// }


// void TurbulentVelocityInletBC::initInflowBC(const boost::filesystem::path& location, const ParameterSet& iniparams) const
// {
//   if (p_.initializer())
//   {
//     OFDictData::dictFile inflowProperties;
//     
//     OFDictData::list& initializers = inflowProperties.addListIfNonexistent("initializers");
//     
//     initializers.push_back( p_.initializer()->type() );
//     OFDictData::dict d;
//     p_.initializer()->addToInitializerList(d, patchName_, p_.velocity().representativeValue(), iniparams);
//     initializers.push_back(d);
//     
//     // then write to file
//     inflowProperties.write( location / "constant" / "inflowProperties" );
//     
//     OFcase().executeCommand(location, "initInflowGenerator");
//   }
// }
  
PressureOutletBC::PressureOutletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
 type_="patch";
}

void PressureOutletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  //p_.phasefractions()->addIntoDictionaries(dictionaries);

  if (p_.fixMeanValue() && (OFversion()!=160))
  {
    OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
    controlDict.getList("libs").push_back( OFDictData::data("\"libfixedMeanValueBC.so\"") );
  }
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
      
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      if (p_.prohibitInflow())
      {
	BC["type"]=OFDictData::data("inletOutlet");
	BC["inletValue"]=OFDictData::data("uniform ( 0 0 0 )");
	BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
      }
      else
      {
	BC["type"]=OFDictData::data("zeroGradient");
	BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
      }
    }
    else if ( 
      (field.first=="T") 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]="zeroGradient";
    }
    else if ( 
      ( (field.first=="p") || (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      if (p_.fixMeanValue())
      {
	BC["type"]=OFDictData::data("fixedMeanValue");
	BC["meanValue"]=OFDictData::data( p_.pressure() );
	BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
      }
      else
      {
	BC["type"]=OFDictData::data("fixedValue");
	BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
      }
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if 
    (
      (
	(field.first=="k") ||
	(field.first=="epsilon") ||
	(field.first=="omega") ||
	(field.first=="nut") ||
	(field.first=="nuSgs") ||
	(field.first=="nuTilda")
      )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
/*	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)*/
	  ))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	  
    }
  }
}

PotentialFreeSurfaceBC::PotentialFreeSurfaceBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict
)
: BoundaryCondition(c, patchName, boundaryDict)
{
 type_="patch";
}

void PotentialFreeSurfaceBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  //p_.phasefractions()->addIntoDictionaries(dictionaries);

  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
      
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("pressureInletOutletParSlipVelocity");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      (field.first=="T") 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]="zeroGradient";
    }
    else if ( 
      ( (field.first=="p_gh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
	BC["type"]=OFDictData::data("waveSurfacePressure");
	BC["value"]=OFDictData::data("uniform 0");
    }
    else if ( 
      ( (field.first=="p") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
	BC["type"]=OFDictData::data("zeroGradient");
	//BC["value"]=OFDictData::data("uniform 0");
    }
    else if 
    (
      (
	(field.first=="k") ||
	(field.first=="epsilon") ||
	(field.first=="omega") ||
	(field.first=="nut") ||
	(field.first=="nuSgs") ||
	(field.first=="nuTilda")
      )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
/*	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)*/
	  ))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	  
    }
  }
}


MeshMotionBC::MeshMotionBC()
{
}

MeshMotionBC::~MeshMotionBC()
{
}

void MeshMotionBC::addIntoDictionaries(OFdicts& dictionaries) const
{}

bool NoMeshMotion::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if 
    ( 
      ((fieldname=="displacement")||(fieldname == "motionU"))
      && 
      (get<0>(fieldinfo)==vectorField) 
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform (0 0 0)");
      return true;
    }
    else 
      return false;
}

MeshMotionBC* NoMeshMotion::clone() const
{
  return new NoMeshMotion(*this);
}

NoMeshMotion noMeshMotion;



CAFSIBC::CAFSIBC(const Parameters& p)
: p_(p)
{
}

CAFSIBC::~CAFSIBC()
{
}

void CAFSIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libFEMDisplacementBC.so\"") );
}

bool CAFSIBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "motionU")
  {
    BC["prescribeMotionVelocity"] = OFDictData::data(true);
  }
  if ( (fieldname == "pointDisplacement") || (fieldname == "motionU") )
  {
    BC["type"]= OFDictData::data("FEMDisplacement");
    BC["FEMCaseDir"]=  OFDictData::data(std::string("\"")+p_.FEMScratchDir().c_str()+"\"");
    BC["pressureScale"]=  OFDictData::data(p_.pressureScale());
    BC["minPressure"]=  OFDictData::data(p_.clipPressure());
    BC["nSmoothIter"]=  OFDictData::data(4);
    BC["wallCollisionCheck"]=  OFDictData::data(true);
    if (p_.oldPressure().get())
    {
      std::ostringstream oss;
      oss<<"uniform "<<*p_.oldPressure();
      BC["oldPressure"] = OFDictData::data(oss.str());
    }
    BC["value"]=OFDictData::data("uniform (0 0 0)");
    
    OFDictData::list relaxProfile;
    if (p_.relax().which()==0)
    {
      OFDictData::list cp;
      cp.push_back(0.0);
      cp.push_back( boost::get<double>(p_.relax()) );
      relaxProfile.push_back( cp );
    }
    else
    {
      BOOST_FOREACH(const RelaxProfile::value_type& rp,  boost::get<RelaxProfile>(p_.relax()) )
      {
	OFDictData::list cp;
	cp.push_back(rp.first);
	cp.push_back(rp.second);
	relaxProfile.push_back(cp);
      }
    }
    BC["relax"]=  relaxProfile;
    
    return true;
  }
  return false;
}

MeshMotionBC* CAFSIBC::clone() const
{
  return new CAFSIBC(*this);
}


WallBC::WallBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, Parameters const& p)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
  type_="wall";
}

void WallBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  p_.meshmotion()->addIntoDictionaries(dictionaries);
  BoundaryCondition::addIntoDictionaries(dictionaries);
}

void WallBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC = dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    // velocity
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      if (p_.rotating())
      {
	BC["type"]=OFDictData::data("rotatingWallVelocity");
	BC["origin"]=OFDictData::to_OF(p_.CofR());
	double om=norm(p_.wallVelocity(), 2);
	BC["axis"]=OFDictData::to_OF(p_.wallVelocity()/om);
	BC["omega"]=lexical_cast<string>(om);
// 	BC["value"]="uniform (0 0 0)"; //!dont include value, will trigger evaluation
      }
      else
      {
	BC["type"]=OFDictData::data("fixedValue");
	BC["value"]=OFDictData::data("uniform "+OFDictData::to_OF(p_.wallVelocity()));
      }
    }
    
    // pressure
    else if ( (field.first=="p") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    
    // temperature
    else if ( 
      (field.first=="T") 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]="zeroGradient";
    }
    
    // pressure
    else if ( ( (field.first=="p_rgh") || (field.first=="pd") ) 
	      && (get<0>(field.second)==scalarField) )
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("fixedFluxPressure");
      else
	BC["type"]=OFDictData::data("buoyantPressure");
//       BC["type"]=OFDictData::data("buoyantPressure");
    }
    
    // turbulence quantities, should be handled by turbulence model
    else if ( 
      ( (field.first=="k") || (field.first=="omega") || (field.first=="epsilon") || (field.first=="nut") || (field.first=="nuTilda") ) 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      OFcase().get<turbulenceModel>("turbulenceModel")->addIntoFieldDictionary(field.first, field.second, BC);
    }
    
    // any other scalar field
    else if (get<0>(field.second)==scalarField)
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    
    else
    {
      if (!p_.meshmotion()->addIntoFieldDictionary(field.first, field.second, BC))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
    }
  }
}

void WallBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  BoundaryCondition::addOptionsToBoundaryDict(bndDict);
}

}

