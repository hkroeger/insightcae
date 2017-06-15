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

#include "analysiscaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

#include "gnuplot-iostream.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{
  
defineType(outputFilterFunctionObject);
defineFactoryTable
(
    outputFilterFunctionObject, 
    LIST 
    (  
        OpenFOAMCase& c, 
        const ParameterSet& ps
    ),
    LIST ( c, ps ) 
);
defineStaticFunctionTable(outputFilterFunctionObject, defaultParameters, ParameterSet);

outputFilterFunctionObject::outputFilterFunctionObject(OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, Parameters(ps).name+"outputFilterFunctionObject"),
  p_(ps)
{
}

void outputFilterFunctionObject::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict fod=functionObjectDict();
  fod["enabled"]=true;
  fod["outputControl"]=p_.outputControl;
  fod["outputInterval"]=p_.outputInterval;
  fod["timeStart"]=p_.timeStart;
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.addSubDictIfNonexistent("functions")[p_.name]=fod;
}

void outputFilterFunctionObject::evaluate
(
  OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results, 
  const std::string& shortDescription
) const
{
}


defineType(fieldAveraging);
addToOpenFOAMCaseElementFactoryTable(fieldAveraging);


fieldAveraging::fieldAveraging(OpenFOAMCase& c,  const ParameterSet& ps )
: outputFilterFunctionObject(c, ps),
  p_(ps)
{
}

OFDictData::dict fieldAveraging::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="fieldAverage";
  OFDictData::list libl; libl.push_back("\"libfieldFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;
  fod["enabled"]=true;
  
  OFDictData::list fl;
  BOOST_FOREACH(const std::string& fln, p_.fields)
  {
    fl.push_back(fln);
    OFDictData::dict cod;
    cod["mean"]=true;
    cod["prime2Mean"]=true;
    cod["base"]="time";
    fl.push_back(cod);
  }
  fod["fields"]=fl;
  
  return fod;
}
  
  
  
  
defineType(probes);
addToOpenFOAMCaseElementFactoryTable(probes);

probes::probes(OpenFOAMCase& c,  const ParameterSet& ps )
: outputFilterFunctionObject(c, ps),
  p_(ps)
{
}

OFDictData::dict probes::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="probes";
  OFDictData::list libl; libl.push_back("\"libsampling.so\"");
  fod["functionObjectLibs"]=libl;
  
  OFDictData::list pl;
  BOOST_FOREACH(const arma::mat& lo, p_.probeLocations)
  {
    pl.push_back(OFDictData::vector3(lo));
  }
  fod["probeLocations"]=pl;
  
  OFDictData::list fl; fl.resize(p_.fields.size());
  copy(p_.fields.begin(), p_.fields.end(), fl.begin());
  fod["fields"]=fl;
  
  return fod;
}




defineType(cuttingPlane);
addToOpenFOAMCaseElementFactoryTable(cuttingPlane);

cuttingPlane::cuttingPlane(OpenFOAMCase& c, const ParameterSet& ps)
: outputFilterFunctionObject(c, ps),
  p_(ps)
{
}

OFDictData::dict cuttingPlane::functionObjectDict() const
{
  OFDictData::dict fod;

  fod["type"]="surfaces";
  OFDictData::list l;
  l.assign<string>(list_of<string>("\"libsampling.so\""));
  fod["functionObjectLibs"]=l;
  fod["interpolationScheme"]="cellPoint";

  fod["surfaceFormat"]="vtk";

        // Fields to be sampled
  l.assign<string>(p_.fields);
  fod["fields"]=l;

  OFDictData::dict pd;
  pd["type"]="cuttingPlane";
  pd["planeType"]="pointAndNormal";
  pd["interpolate"]=false;
  OFDictData::dict pand;
  pand["basePoint"]="(0 0 0)";
  pand["normalVector"]="(0 0 1)";
  pd["pointAndNormalDict"]=pand;

  OFDictData::list sl;
  sl.push_back(p_.name);
  sl.push_back(pd);
  fod["surfaces"]=sl;
  
  return fod;
}

   
   
   
defineType(twoPointCorrelation);
addToOpenFOAMCaseElementFactoryTable(twoPointCorrelation);

twoPointCorrelation::twoPointCorrelation(OpenFOAMCase& c, const ParameterSet& ps)
: outputFilterFunctionObject(c, ps),
  p_(ps)
{
}

OFDictData::dict twoPointCorrelation::csysConfiguration() const
{
  OFDictData::data 
    e1=OFDictData::vector3(1,0,0), 
    e2=OFDictData::vector3(0,1,0);
    
  OFDictData::dict csys;
  csys["type"]="cartesian";
  csys["origin"]=OFDictData::vector3(0,0,0);
  if (OFversion()>=230)
  {
    OFDictData::dict top, crd;
    crd["type"]="axesRotation";
    crd["e1"]=e1;
    crd["e2"]=e2;
    csys["coordinateRotation"]=crd;
    top["coordinateSystem"]=csys;
    return top;
  } 
  else
  {
    csys["e1"]=e1;
    csys["e2"]=e2;
    return csys;
  }
}

OFDictData::dict twoPointCorrelation::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="twoPointCorrelation";
  OFDictData::list libl; libl.push_back("\"libLESFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;
  fod["enabled"]=true;
  fod["outputControl"]=p_.outputControl;
  fod["outputInterval"]=p_.outputInterval;
  fod["timeStart"]=p_.timeStart;
  
  fod["p0"]=OFDictData::vector3(p_.p0);
  fod["directionSpan"]=OFDictData::vector3(p_.directionSpan);
  fod["np"]=p_.np;
  fod["homogeneousTranslationUnit"]=OFDictData::vector3(p_.homogeneousTranslationUnit);
  fod["nph"]=p_.nph;

  fod["csys"]=csysConfiguration();
  
  return fod;
}

boost::ptr_vector<arma::mat> twoPointCorrelation::readCorrelations(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& tpcName)
{
  int nk=9;
  
  std::vector<double> t; // time step array
  std::vector<double> profs[nk]; // profiles
  int np=-1;
  
  path fp;
  if (c.OFversion()<170)
    fp=absolute(location)/tpcName;
  else
    fp=absolute(location)/"postProcessing"/tpcName;
  
  TimeDirectoryList tdl=listTimeDirectories(fp);
  
  BOOST_FOREACH(const TimeDirectoryList::value_type& td, tdl)
  {
    std::ifstream f( (td.second/"twoPointCorrelation.dat").c_str());
    while (!f.eof())
    {
      string line;
      getline(f, line);
      if (f.fail()) break;

      if (!starts_with(line, "#"))
      {
	
	// split into component tpcs
	std::vector<string> strs;
	boost::split(strs, line, boost::is_any_of("\t"));
	
	t.push_back(lexical_cast<double>(strs[0]));
	
	if (strs.size()!=(nk+2))
	  throw insight::Exception("Expected "
				    +lexical_cast<string>(nk+1)
				    +" profiles in twoPointCorrelation results but got "
				    +lexical_cast<string>(strs.size()-1)+"!");
	
	for (int k=1; k<=nk; k++)
	{
	  std::vector<string> pts;
	  boost::split(pts, strs[k], boost::is_any_of(" "));
	  if (np<0) 
	    np=pts.size()-1; // trailing space!
	  else if (np!=pts.size()-1)
	    throw insight::Exception("Expected uniform number of sampling point in twoPointCorrelation results!");
	  
	  for (int j=0; j<np; j++)
	  {
	    profs[k-1].push_back(lexical_cast<double>(pts[j]));
	  }
	}
      }
    }
  }
  
  ptr_vector<arma::mat> res;
  res.push_back(new arma::mat(t.data(), t.size(), 1));
  for (int k=0; k<nk; k++) res.push_back(new arma::mat( arma::mat(profs[k].data(), np, t.size()).t() ) );
  
  return res;  
}




defineType(cylindricalTwoPointCorrelation);
addToOpenFOAMCaseElementFactoryTable(cylindricalTwoPointCorrelation);

cylindricalTwoPointCorrelation::cylindricalTwoPointCorrelation(OpenFOAMCase& c, const ParameterSet& ps )
: twoPointCorrelation(c, ps),
  p_(ps)
{
}

OFDictData::dict cylindricalTwoPointCorrelation::csysConfiguration() const
{
  OFDictData::dict csys;
  csys["type"]="cylindrical";
  csys["origin"]=OFDictData::vector3(0,0,0);
  csys["degrees"]=p_.degrees;
  
  OFDictData::data 
    e1=OFDictData::vector3(p_.er), 
    e3=OFDictData::vector3(p_.ez);
    
  if (OFversion()>=230)
  {
    OFDictData::dict top, crd;
    crd["type"]="axesRotation";
    crd["e1"]=e1;
    crd["e3"]=e3;
    csys["coordinateRotation"]=crd;
    top["coordinateSystem"]=csys;
    return top;
  } 
  else
  {
    csys["e1"]=e1;
    csys["e3"]=e3;
    return csys;
  }
  
}




defineType(forces);
addToOpenFOAMCaseElementFactoryTable(forces);

forces::forces(OpenFOAMCase& c,  const ParameterSet& ps)
: OpenFOAMCaseElement(c, Parameters(ps).name+"forces"),
  p_(ps)
{
}

void forces::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict fod;
  fod["type"]="forces";
  OFDictData::list libl; libl.push_back("\"libforces.so\"");
  fod["functionObjectLibs"]=libl;
  fod["log"]=true;
  fod["outputControl"]=p_.outputControl;
  fod["outputInterval"]=p_.outputInterval;
  
  OFDictData::list pl;
  BOOST_FOREACH(const std::string& lo, p_.patches)
  {
    pl.push_back(lo);
  }
  fod["patches"]=pl;
  fod["pName"]=p_.pName;
  fod["UName"]=p_.UName;
  if (OFversion()>=400)
    fod["rho"]=p_.rhoName;
  else
    fod["rhoName"]=p_.rhoName;
  fod["rhoInf"]=p_.rhoInf;
  
  fod["CofR"]=OFDictData::vector3(p_.CofR);
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.addSubDictIfNonexistent("functions")[p_.name]=fod;
}


arma::mat readForcesLine(std::istream& f, int nc_expected, bool& skip)
{
  std::string line;

  getline ( f, line );
//   std::cout<<"RAW: "<<line<<std::endl;
  
  if ( f.fail() ) return arma::mat();

  if ( starts_with ( line, "#" ) ) { skip=true; return arma::mat(); };

  erase_all ( line, "(" );
  erase_all ( line, ")" );
  replace_all ( line, ",", " " );
  replace_all ( line, "\t", " " );
  while (line.find("  ")!=std::string::npos)
  {
    replace_all ( line, "  ", " " );
  }
//   std::cout<<"CLEAN: "<<line<<std::endl;

  std::vector<string> strs;
  boost::split ( strs, line, boost::is_any_of ( " " ) );

  if ( strs.size() != nc_expected ) 
  {
//       std::cout<<"found "<<strs.size()<<" fields, expected "<<nc_expected<<std::endl;
      return arma::mat();
  }

  arma::mat row;
  row.set_size ( 1, strs.size() );

  int k=0;
  BOOST_FOREACH ( const string& e, strs )
  {
    row ( 0, k++ ) =lexical_cast<double> ( e );
  }
  
  return row;
}

arma::mat forces::readForces ( const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& foName )
{
  arma::mat fl;

  path fp;
  if ( c.OFversion() <170 )
    fp=absolute ( location ) /foName;
  else
    fp=absolute ( location ) /"postProcessing"/foName;

  TimeDirectoryList tdl=listTimeDirectories ( fp );

  int ncexp=13;
  if ( c.OFversion() >=230 ) ncexp=19;
  if ( c.OFversion() >=300 ) ncexp=10;

  BOOST_FOREACH ( const TimeDirectoryList::value_type& td, tdl )
  {
    std::ifstream f, f2;
    if ( c.OFversion() >=300 )
      {
        f.open ( ( td.second/"force.dat" ).c_str() );
        f2.open ( ( td.second/"moment.dat" ).c_str() );
      }
    else
      {
        f.open ( ( td.second/"forces.dat" ).c_str() );
      }

    while ( !f.eof() )
      {

        bool skip=false;

        arma::mat row;
        if ( c.OFversion() >=300 )
          {
            arma::mat r1=readForcesLine ( f, ncexp, skip );
            arma::mat r2=readForcesLine ( f2, ncexp, skip );
//             std::cout<<r1<<r2<<std::endl;
            if ( (r1.n_cols==0) || (r2.n_cols==0) )
            {
                if ( !skip ) break;
            }
            else
            {
                // make compatible with earlier OF versions
                r1.shed_cols(1,3); // remove total force
                r2.shed_cols(1,3); // remove total moment
                r2.shed_col(0); // remove time column
//                 if (r1.n_cols==7) // append porosity columns, if not present
//                 {
//                     arma::mat rm=arma::join_rows(r1, arma::zeros(1,3));
//                     r1=rm;
//                 }
//                 if (r2.n_cols==7) // append porosity columns, if not present
//                 {
//                     arma::mat rm=arma::join_rows(r2, arma::zeros(1,3));
//                     r2=rm;
//                 }
                
                row=arma::join_rows ( r1, r2 );
//     std::cout<<"r="<<row<<std::endl;
                
            }
          }
        else
          {
            row=readForcesLine ( f, ncexp, skip );
            if ( row.n_cols==0 )
            {
                if ( !skip ) break;
            }
            else
            {
                if ( c.OFversion() >=220 )
                {
                    // remove porous forces
                    row.shed_cols ( 16,18 );
                    row.shed_cols ( 7,9 );
                } 
            }
        }

        if (row.n_cols>0)
        {
         if ( fl.n_rows==0 )
           {
             fl=row;
           }
         else
           {
             fl.resize ( fl.n_rows+1, fl.n_cols );
             fl.row ( fl.n_rows-1 ) = row;
           }
        }
      }
  }

//   if ( c.OFversion() >=220 )
//     {
//       // remove porous forces
//       fl.shed_cols ( 16,18 );
//       fl.shed_cols ( 7,9 );
//     }

  return fl;
}

defineType(extendedForces);
addToOpenFOAMCaseElementFactoryTable(extendedForces);

extendedForces::extendedForces(OpenFOAMCase& c, const ParameterSet& ps)
: forces(c, ps)
{
}

void extendedForces::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict fod;
  fod["type"]="extendedForces";
  OFDictData::list libl; libl.push_back("\"libextendedForcesFunctionObject.so\"");
  fod["functionObjectLibs"]=libl;
  fod["log"]=true;
  fod["outputControl"]=p_.outputControl;
  fod["outputInterval"]=p_.outputInterval;
  
  OFDictData::list pl;
  BOOST_FOREACH(const std::string& lo, p_.patches)
  {
    pl.push_back(lo);
  }
  fod["patches"]=pl;
  fod["pName"]=p_.pName;
  fod["UName"]=p_.UName;
  fod["rhoName"]=p_.rhoName;
  fod["rhoInf"]=p_.rhoInf;
  
  fod["CofR"]=OFDictData::vector3(p_.CofR);
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.addSubDictIfNonexistent("functions")[p_.name]=fod;
}
  
CorrelationFunctionModel::CorrelationFunctionModel()
: B_(1.0), omega_(1.0)
{}

int CorrelationFunctionModel::numP() const
{
  return 2;
}

void CorrelationFunctionModel::setParameters(const double* params)
{
  B_=max(0.1, params[0]);
  omega_=params[1];
}

void CorrelationFunctionModel::setInitialValues(double* params) const
{
  params[0]=0.1;
  params[1]=0.1;
}

arma::mat CorrelationFunctionModel::weights(const arma::mat& x) const
{
  return exp( -x / max(x.col(0)) );
}

arma::mat CorrelationFunctionModel::evaluateObjective(const arma::mat& x) const
{
  //cout<<B_<<" "<<omega_<<" "<<x.col(0)<<endl;
  return exp(-B_*x.col(0)) % ( cos(omega_*x.col(0)) );
}

double CorrelationFunctionModel::lengthScale() const
{
  return B_ / ( pow(B_, 2)+pow(omega_, 2) );
}

template<>
const char * LinearTPCArray::cmptNames[] = 
{ "xx", "xy", "xz",
  "yx", "yy", "yz",
  "zx", "zy", "zz"
};

template<>
typename twoPointCorrelation::Parameters LinearTPCArray::getTanParameters(int i) const
{
  return 
    typename twoPointCorrelation::Parameters(twoPointCorrelation::Parameters()
//       .set_p0(vec3(p_.x, r_.back(), p_.z))
      .set_p0( p_.p0 + r_.back()*p_.e_rad )
//       .set_directionSpan(vec3(0, 0, p_.tanSpan)) 
      .set_directionSpan( p_.tanSpan*p_.e_tan ) 
      .set_np(p_.np)
//       .set_homogeneousTranslationUnit(vec3(0, 0, p_.tanSpan/double(p_.nph)))
      .set_homogeneousTranslationUnit( (p_.tanSpan/double(p_.nph))*p_.e_tan )
      .set_nph( p_.nph )

      .set_name(p_.name+"_tan_"+lexical_cast<std::string>(i))
      .set_outputControl("timeStep")
      .set_timeStart( p_.timeStart )
    );
}

template<>
typename twoPointCorrelation::Parameters LinearTPCArray::getAxParameters(int i) const
{
  return
    typename twoPointCorrelation::Parameters(twoPointCorrelation::Parameters()
//       .set_p0(vec3(p_.x, r_.back(), p_.z))
      .set_p0( p_.p0 + r_.back()*p_.e_rad )
//       .set_directionSpan(vec3(p_.axSpan, 0, 0)) 
      .set_directionSpan( p_.axSpan*p_.e_ax ) 
      .set_np(p_.np)
//       .set_homogeneousTranslationUnit(vec3(0, 0, p_.tanSpan/double(p_.nph)))
      .set_homogeneousTranslationUnit( (p_.tanSpan/double(p_.nph))*p_.e_tan )
      .set_nph(p_.nph)

      .set_name(p_.name+"_ax_"+lexical_cast<std::string>(i))
      .set_outputControl("timeStep")
      .set_timeStart( p_.timeStart )
    );
}

template<>
std::string LinearTPCArray::axisTitleTan() const
{
   return "Tangential distance [length]";
}

template<>
std::string LinearTPCArray::axisTitleAx() const
{
   return "Axial distance [length]";
}

template<>
const char * RadialTPCArray::cmptNames[] = 
{ "xx", "xy", "xz",
  "yx", "yy", "yz",
  "zx", "zy", "zz"
};

template<>
typename cylindricalTwoPointCorrelation::Parameters RadialTPCArray::getTanParameters(int i) const
{
  return 
    typename cylindricalTwoPointCorrelation::Parameters(cylindricalTwoPointCorrelation::Parameters()
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)

//       .set_p0(vec3(r_.back(), 0, p_.x))
      .set_p0( p_.p0 + r_.back()*p_.e_rad )
//       .set_directionSpan(vec3(0, p_.tanSpan, 0)) 
      .set_directionSpan( p_.tanSpan*p_.e_tan ) 
      .set_np(p_.np)
//       .set_homogeneousTranslationUnit(vec3(0, 2.*M_PI/double(p_.nph), 0))
      .set_homogeneousTranslationUnit( (2.*M_PI/double(p_.nph))*p_.e_tan )
      .set_nph( p_.nph )

      .set_name(p_.name+"_tan_"+lexical_cast<std::string>(i))
      .set_outputControl("timeStep")
      .set_timeStart( p_.timeStart )
    );
}

template<>
typename cylindricalTwoPointCorrelation::Parameters RadialTPCArray::getAxParameters(int i) const
{
  return
    typename cylindricalTwoPointCorrelation::Parameters(cylindricalTwoPointCorrelation::Parameters()
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)

//       .set_p0(vec3(r_.back(), 0, p_.x))
      .set_p0( p_.p0 + r_.back()*p_.e_rad )
//       .set_directionSpan(vec3(0, 0, p_.axSpan)) 
      .set_directionSpan( p_.axSpan*p_.e_ax ) 
      .set_np(p_.np)
//       .set_homogeneousTranslationUnit(vec3(0, 2.*M_PI/double(p_.nph), 0))
      .set_homogeneousTranslationUnit( (2.*M_PI/double(p_.nph))*p_.e_tan )
      .set_nph(p_.nph)

      .set_name(p_.name+"_ax_"+lexical_cast<std::string>(i))
      .set_outputControl("timeStep")
      .set_timeStart( p_.timeStart )
    );
}

template<>
std::string RadialTPCArray::axisTitleTan() const
{
   return "Angle [rad]";
}

template<>
std::string RadialTPCArray::axisTitleAx() const
{
   return "Axial distance [length]";
}

const char LinearTPCArrayTypeName[] = "LinearTPCArray";
// defineType(LinearTPCArray);
template<> const std::string LinearTPCArray::typeName( LinearTPCArray::typeName_() );
addToFactoryTable(OpenFOAMCaseElement, LinearTPCArray);
addToStaticFunctionTable(OpenFOAMCaseElement, LinearTPCArray, defaultParameters);
addToStaticFunctionTable(OpenFOAMCaseElement, LinearTPCArray, category);
addToFactoryTable(outputFilterFunctionObject, LinearTPCArray);
addToStaticFunctionTable(outputFilterFunctionObject, LinearTPCArray, defaultParameters);

const char RadialTPCArrayTypeName[] = "RadialTPCArray";
// defineType(RadialTPCArray);
template<> const std::string RadialTPCArray::typeName( RadialTPCArray::typeName_() );
addToFactoryTable(OpenFOAMCaseElement, RadialTPCArray);
addToStaticFunctionTable(OpenFOAMCaseElement, RadialTPCArray, defaultParameters);
addToStaticFunctionTable(OpenFOAMCaseElement, RadialTPCArray, category);
addToFactoryTable(outputFilterFunctionObject, RadialTPCArray);
addToStaticFunctionTable(outputFilterFunctionObject, RadialTPCArray, defaultParameters);

}
