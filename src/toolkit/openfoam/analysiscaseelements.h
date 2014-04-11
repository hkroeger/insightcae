/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#ifndef INSIGHT_ANALYSISCASEELEMENTS_H
#define INSIGHT_ANALYSISCASEELEMENTS_H

#include "basiccaseelements.h"
#include "base/resultset.h"

namespace insight {

  
  
  
/** ===========================================================================
 * Base class for function objects, that understand the output* options
 */
class outputFilterFunctionObject
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (name, std::string, "unnamed")
    (timeStart, double, 0.0)
    (outputControl, std::string, "outputTime")    
    (outputInterval, double, 1.0)
  )
  
protected:
  Parameters p_;
  
public:
  outputFilterFunctionObject(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual OFDictData::dict functionObjectDict() const =0;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};




/** ===========================================================================
 * function object front end for field fieldAveraging
 */
class fieldAveraging
: public outputFilterFunctionObject
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, outputFilterFunctionObject::Parameters,
    (fields, std::vector<std::string>, std::vector<std::string>())
  )
  
protected:
  Parameters p_;
  
public:
  fieldAveraging(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual OFDictData::dict functionObjectDict() const;
};

/** ===========================================================================
 * function object front end for probes
 */
class probes
: public outputFilterFunctionObject
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, outputFilterFunctionObject::Parameters,
    (fields, std::vector<std::string>, std::vector<std::string>())
    (probeLocations, std::vector<arma::mat>, std::vector<arma::mat>())
  )
  
protected:
  Parameters p_;
  
public:
  probes(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual OFDictData::dict functionObjectDict() const;
};




/** ===========================================================================
 * function object front end for live cutting plane sampling during run
 */
class cuttingPlane
: public outputFilterFunctionObject
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, outputFilterFunctionObject::Parameters,
    (fields, std::vector<std::string>, std::vector<std::string>())
    (basePoint, arma::mat, vec3(0,0,0))
    (normal, arma::mat, vec3(0,0,1))
    (interpolate, bool, false)
  )
  
protected:
  Parameters p_;
  
public:
  cuttingPlane(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual OFDictData::dict functionObjectDict() const;
};



/** ===========================================================================
 * function object front end for two-point correlation sampling
 */
class twoPointCorrelation
: public outputFilterFunctionObject
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, outputFilterFunctionObject::Parameters,
    (p0, arma::mat, vec3(0,0,0))
    (directionSpan, arma::mat, vec3(1,0,0))
    (np, int, 50)
    (homogeneousTranslationUnit, arma::mat, vec3(0,1,0))
    (nph, int, 1)
  )
  
protected:
  Parameters p_;
  
public:
  twoPointCorrelation(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual OFDictData::dict functionObjectDict() const;
  virtual OFDictData::dict csysConfiguration() const;

  inline const std::string& name() const { return p_.name(); }
  static boost::ptr_vector<arma::mat> readCorrelations(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& tpcName);
};




/** ===========================================================================
 * front end for sampling of two-point correlations in cylindrical CS
 */
class cylindricalTwoPointCorrelation
: public twoPointCorrelation
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, twoPointCorrelation::Parameters,
    (ez, arma::mat, vec3(0,0,1))
    (er, arma::mat, vec3(1,0,0))
    (degrees, bool, false)
  )
  
protected:
  Parameters p_;
  
public:
  cylindricalTwoPointCorrelation(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual OFDictData::dict csysConfiguration() const;
};

class forces
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (name, std::string, "forces")
    (patches, std::vector<std::string>, std::vector<std::string>())
    (pName, std::string, "p")
    (UName, std::string, "U")
    (rhoName, std::string, "rhoInf")
    (rhoInf, double, 1.0)
    (outputControl, std::string, "timeStep")    
    (outputInterval, double, 10.0)
    (CofR, arma::mat, vec3(0,0,0))
  )
  
protected:
  Parameters p_;
  
public:
  forces(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  
  static arma::mat readForces(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& foName);
};

  
class CorrelationFunctionModel
: public RegressionModel
{
public:
  double B_, omega_;
  
  CorrelationFunctionModel();
  virtual int numP() const;
  virtual void setParameters(const double* params);
  virtual arma::mat weights(const arma::mat& x) const;
  virtual arma::mat evaluateObjective(const arma::mat& x) const;
  
  double lengthScale() const;
};
  
class RadialTPCArray
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (name_prefix, std::string, "tpc")
    (timeStart, double, 0.0)
    (outputControl, std::string, "outputTime")    
    (outputInterval, double, 10.0)
    (x, double, 0.0)
    (tanSpan, double, M_PI)
    (axSpan, double, 1.0)
    (np, int, 50)
    (nph, int, 8)
    (R, double, 1.0)
  )
  
  static const char * cmptNames[];
  
protected:
  Parameters p_;
  std::vector<double> r_;
  boost::ptr_vector<cylindricalTwoPointCorrelation> tpc_ax_;
  boost::ptr_vector<cylindricalTwoPointCorrelation> tpc_tan_;
  
public:
  RadialTPCArray(OpenFOAMCase& c, Parameters const &p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  virtual void evaluate(OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results) const;
  virtual void evaluateSingle
  (
    OpenFOAMCase& cm, const boost::filesystem::path& location, 
    ResultSetPtr& results, 
    const std::string& name_prefix,
    double span,
    const std::string& axisLabel,
    const boost::ptr_vector<cylindricalTwoPointCorrelation>& tpcarray,
    const std::string& shortDescription
  ) const;
};

}

#endif // INSIGHT_ANALYSISCASEELEMENTS_H
