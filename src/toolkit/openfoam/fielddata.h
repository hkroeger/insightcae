#ifndef FIELDDATA_H
#define FIELDDATA_H

#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamdict.h"


namespace insight
{

/**
 * Interface which wraps different types of prescribing field data on boundaries.
 * Works together with OpenFOAM class "FieldDataProvider".
 */

class FieldData
{
public:

#include "fielddata__FieldData__Parameters.h"

/*
PARAMETERSET>>> FieldData Parameters

n_cmpt = int 3 "Number of components" *hidden

fielddata=selectablesubset {{

 uniformSteady
 set {
     value=vector (1 0 0) "Constant steady field value"
 }

 uniform
 set {
   values=array
   [
    set {
     time=double 0 "Time instant"
     value=vector (1 0 0) "Field value"
    } "Time instant data"
   ] * 1  "Array of time instants"
 }

 linearProfile
 set {
   values=array
   [
    set {
     time=double 0 "Time instant"
     profile=path "profile.dat" "Path to file with tabulated profile for this time instant. Needs to contain one column per component."
    } "Time instant data"
   ] * 1  "Array of time instants"


   p0=vector (0 0 0) "Origin of sampling axis"
   ep=vector (1 0 0) "Direction of sampling axis"
 }

 radialProfile
 set {
   values=array
   [
    set {
     time=double 0 "Time instant"
     profile=path "profile.dat" "Path to file with tabulated profile for this time instant. Needs to contain one column per component."
    } "Time instant data"
   ] * 1  "Array of time instants"

   p0=vector (0 0 0) "Origin of sampling axis"
   ep=vector (1 0 0) "Direction of sampling axis"
 }

 fittedProfile
 set {
   values=array
   [
    set {
     time=double 0 "Time instant"
     component_coeffs=array
      [
       vector (1 1) "Coefficients of profile polynomial"
      ] * 3 "Sets of polynomial coefficients for each tensor component"
    } "Time instant data"
   ] * 1  "Array of time instants"

   p0=vector (0 0 0) "Origin of sampling axis"
   ep=vector (1 0 0) "Direction of sampling axis"
 }

}} uniformSteady "Specification of field value"
<<<PARAMETERSET
*/

protected:
  Parameters p_;

  double representativeValueMag_, maxValueMag_;

  void calcValues(const boost::filesystem::path& casedir);

  /**
   * return some representative (average) value of the prescribed data.
   * Required e.g. for deriving turbulence qtys when velocity distributions are prescribed.
   */
  double calcRepresentativeValueMag(const boost::filesystem::path& casedir) const;

  /**
   * return the maximum magnitude of the value throughout all precribed times
   */
  double calcMaxValueMag(const boost::filesystem::path& casedir) const;

public:


  static Parameters uniformSteady(double uniformSteadyValue);

  /**
   * sets all parameters for the most simple type of field data description (uniform, steady scalar value)
   */
  FieldData(double uniformSteadyValue);

  static Parameters uniformSteady(double uniformSteadyX, double uniformSteadyY, double uniformSteadyZ);
  static Parameters uniformSteady(const arma::mat& uniformSteadyValue);

  /**
   * sets all parameters for the most simple type of field data description (uniform, steady value)
   */
  FieldData(const arma::mat& uniformSteadyValue);

  /**
   * takes config from a parameterset
   */
  FieldData(const ParameterSet& p, const boost::filesystem::path& casedir);

  /**
   * returns according dictionary entry for OF
   */
  OFDictData::data sourceEntry() const;

  void setDirichletBC(OFDictData::dict& BC) const;

  /**
   * return some representative (average) value of the prescribed data.
   * Required e.g. for deriving turbulence qtys when velocity distributions are prescribed.
   */
  inline double representativeValueMag() const { return representativeValueMag_; }

  /**
   * return the maximum magnitude of the value throughout all precribed times
   */
  inline double maxValueMag() const { return maxValueMag_; }

  /**
   * returns a proper parameterset for this entity
   * @reasonable_value: the number of components determines the rank of the field
   */
  static Parameter* defaultParameter(const arma::mat& reasonable_value, const std::string& description="Origin of the prescribed value");

  /**
   * insert graphs with prescribed profiles into result set (only, if profiles were prescribed, otherwise inserts nothing)
   * @param results ResultSet to which the data is added
   * @param name Name of result entry
   * @param descr Description
   * @param qtylabel Label (formula symbol) of the quantity. Will be interpreted as latex math expression
   */
  void insertGraphsToResultSet(ResultSetPtr results, const boost::filesystem::path& exepath, const std::string& name, const std::string& descr, const std::string& qtylabel) const;
};

}

#endif // FIELDDATA_H
