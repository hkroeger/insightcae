#include "setfields.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/ofdicts.h"


namespace insight {




namespace setFieldOps
{

setFieldOperator::setFieldOperator(const OpenFOAMCase& c, ParameterSetInput ip)
    : c_(c), p_(ip.forward<Parameters>())
{
}

setFieldOperator::~setFieldOperator()
{
}




fieldToCellOperator::fieldToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip)
: setFieldOperator(c, ip.forward<Parameters>())
{}

void fieldToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["fieldName"]=p().fieldName;
  opdict["min"]=p().min;
  opdict["max"]=p().max;

  OFDictData::list fve;
  for (const auto& fvs: p().fieldValues)
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( "fieldToCell" );
  setFieldDict.getList("regions").push_back( opdict );

}





boxToCellOperator::boxToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip )
: setFieldOperator(c, ip.forward<Parameters>())
{}

void boxToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["box"]=OFDictData::toString(OFDictData::vector3(p().min))
                  + OFDictData::toString(OFDictData::vector3(p().max));

  OFDictData::list fve;
  for (const auto& fvs: p().fieldValues)
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( "boxToCell" );
  setFieldDict.getList("regions").push_back( opdict );
}




cellToCellOperator::cellToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip )
: setFieldOperator(c, ip.forward<Parameters>())
{}

void cellToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["set"]=p().cellSet;

  OFDictData::list fve;
  for (const auto& fvs: p().fieldValues)
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( "cellToCell" );
  setFieldDict.getList("regions").push_back( opdict );
}





zoneToCellOperator::zoneToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip )
    : setFieldOperator(c, ip.forward<Parameters>())
{}

void zoneToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
    OFDictData::dict opdict;
    if (c_.OFversion()<060505)
        opdict["name"]=p().cellZone;
    else
        opdict["zone"]=p().cellZone;

    OFDictData::list fve;
    for (const auto& fvs: p().fieldValues)
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
