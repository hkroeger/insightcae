/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INSIGHT_PIPE_H
#define INSIGHT_PIPE_H

#include "base/linearalgebra.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/basiccaseelements.h"

namespace insight {
  
class CorrelationFunctionModel
: public RegressionModel
{
public:
  double B_, omega_;
  
  CorrelationFunctionModel();
  virtual int numP() const;
  virtual void setParameters(const double* params);
  virtual arma::mat evaluateObjective(const arma::mat& x) const;
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

class PipeBase 
: public OpenFOAMAnalysis
{
protected:
  std::string cycl_in_, cycl_out_;
  
public:
  declareType("Pipe Flow Test Case");
  
  PipeBase(const NoParameters&);
  ~PipeBase();
  
  virtual ParameterSet defaultParameters() const;
  
  std::string cyclPrefix() const;
  virtual double calcLc(const ParameterSet& p) const;
  virtual int calcnc(const ParameterSet& p) const;
  virtual int calcnr(const ParameterSet& p) const;
  virtual double calcgradr(const ParameterSet& p) const;
  virtual double calcywall(const ParameterSet& p) const;
  virtual double calcRe(const ParameterSet& p) const;
  virtual double calcUbulk(const ParameterSet& p) const;
  virtual double calcT(const ParameterSet& p) const;

  virtual void createMesh
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );
  
  virtual void createCase
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );

  
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm, const ParameterSet& p);
  
};




class PipeCyclic
: public PipeBase
{
public:
  declareType("Pipe Flow Test Case (Axial Cyclic)");
  
  PipeCyclic(const NoParameters&);
  
  virtual void createMesh
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
  
};


class PipeInflow
: public PipeBase
{
public:
  declareType("Pipe Flow Test Case (Inflow Generator)");
  
  PipeInflow(const NoParameters&);
  
  virtual void createMesh
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
  
};


}

#endif // INSIGHT_PIPE_H
