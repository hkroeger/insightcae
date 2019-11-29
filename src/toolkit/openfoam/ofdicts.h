#ifndef INSIGHT_OFDICTS_H
#define INSIGHT_OFDICTS_H

#include "base/boost_include.h"
#include "base/parameters/pathparameter.h"
#include "openfoam/openfoamdict.h"

namespace insight {

enum FieldType
{
  scalarField,
  vectorField,
  symmTensorField
};



enum FieldGeoType
{
  volField,
  pointField,
  tetField
};



extern const OFDictData::dimensionSet dimPressure;
extern const OFDictData::dimensionSet dimKinPressure;
extern const OFDictData::dimensionSet dimKinEnergy;
extern const OFDictData::dimensionSet dimVelocity;
extern const OFDictData::dimensionSet dimLength;
extern const OFDictData::dimensionSet dimDensity;
extern const OFDictData::dimensionSet dimless;
extern const OFDictData::dimensionSet dimKinViscosity;
extern const OFDictData::dimensionSet dimDynViscosity;
extern const OFDictData::dimensionSet dimTemperature;
extern const OFDictData::dimensionSet dimCurrent;



typedef std::vector<double> FieldValue;
typedef boost::fusion::tuple<FieldType, OFDictData::dimensionSet, FieldValue, FieldGeoType > FieldInfo;
typedef std::map<std::string, FieldInfo> FieldList;


struct OFdicts
: public boost::ptr_map<std::string, OFDictData::dictFile>
{
  std::map< std::shared_ptr<PathParameter>, boost::filesystem::path > additionalInputFiles_;

  boost::filesystem::path insertAdditionalInputFile( std::shared_ptr<PathParameter> file );

  OFDictData::dictFile& addFieldIfNonexistent(const std::string& key, const FieldInfo& fi);
  OFDictData::dictFile& lookupDict(const std::string& key, bool createIfNonexistent = true);
};


} // namespace insight

#endif // INSIGHT_OFDICTS_H
