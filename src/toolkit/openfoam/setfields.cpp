#include "setfields.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/ofdicts.h"


namespace insight {




namespace setFieldOps
{

setFieldOperator::setFieldOperator(const OpenFOAMCase& c, ParameterSet const& p)
: c_(c), p_(p)
{
}

setFieldOperator::~setFieldOperator()
{
}




fieldToCellOperator::fieldToCellOperator(const OpenFOAMCase& c, ParameterSet const& p)
: setFieldOperator(c, p),
  p_(p)
{}

void fieldToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["fieldName"]=p_.fieldName;
  opdict["min"]=p_.min;
  opdict["max"]=p_.max;

  OFDictData::list fve;
  for (const auto& fvs: p_.fieldValues)
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( "fieldToCell" );
  setFieldDict.getList("regions").push_back( opdict );

}





boxToCellOperator::boxToCellOperator(const OpenFOAMCase& c, ParameterSet const& p )
: setFieldOperator(c, p),
  p_(p)
{}

void boxToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["box"]=OFDictData::to_OF(p_.min) + OFDictData::to_OF(p_.max);

  OFDictData::list fve;
  for (const auto& fvs: p_.fieldValues)
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( "boxToCell" );
  setFieldDict.getList("regions").push_back( opdict );
}




cellToCellOperator::cellToCellOperator(const OpenFOAMCase& c, ParameterSet const& p )
: setFieldOperator(c, p),
  p_(p)
{}

void cellToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["set"]=p_.cellSet;

  OFDictData::list fve;
  for (const auto& fvs: p_.fieldValues)
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( "cellToCell" );
  setFieldDict.getList("regions").push_back( opdict );
}





zoneToCellOperator::zoneToCellOperator(const OpenFOAMCase& c, ParameterSet const& p )
    : setFieldOperator(c, p),
      p_(p)
{}

void zoneToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
    OFDictData::dict opdict;
    if (c_.OFversion()<060505)
        opdict["name"]=p_.cellZone;
    else
        opdict["zone"]=p_.cellZone;

    OFDictData::list fve;
    for (const auto& fvs: p_.fieldValues)
    {
      fve.push_back( fvs );
    }
    opdict["fieldValues"]=fve;
    setFieldDict.getList("regions").push_back( "zoneToCell" );
    setFieldDict.getList("regions").push_back( opdict );
}




}




void setFields(
        const OpenFOAMCase& ofc,
        const boost::filesystem::path& location,
        const setFieldOps::setFieldOperator::Parameters::fieldValues_type& defaultValues,
        const std::vector<std::shared_ptr<setFieldOps::setFieldOperator> >& ops)
{
  using namespace setFieldOps;

  OFDictData::dictFile setFieldsDict;

  OFDictData::list& dvl = setFieldsDict.getList("defaultFieldValues");
  for ( const auto& dv: defaultValues)
  {
    dvl.push_back( dv );
  }

  setFieldsDict.getList("regions");
  for ( const auto& op: ops)
  {
    op->addIntoDictionary(setFieldsDict);
  }

  // then write to file
  boost::filesystem::path dictpath = location / "system" / "setFieldsDict";
  if (!exists(dictpath.parent_path()))
  {
    boost::filesystem::create_directories(dictpath.parent_path());
  }

  {
    std::ofstream f(dictpath.c_str());
    writeOpenFOAMDict(f, setFieldsDict, boost::filesystem::basename(dictpath));
  }

  ofc.executeCommand(location, "setFields");
}



} // namespace insight
