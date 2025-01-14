#ifndef INSIGHT_SETFIELDS_H
#define INSIGHT_SETFIELDS_H

#include <memory>
#include <vector>

#include "base/boost_include.h"
#include "setfields__setFieldOperator__Parameters_headers.h"

namespace insight {

namespace OFDictData { class dict; }
class OpenFOAMCase;


#ifndef SWIG
namespace setFieldOps
{


class setFieldOperator
{
public:
#include "setfields__setFieldOperator__Parameters.h"
/*
PARAMETERSET>>> setFieldOperator Parameters

fieldValues = array [ string "" "field value specification" ]*0 "list of field value specifications"

createGetter
<<<PARAMETERSET
*/

protected:
  const OpenFOAMCase& c_;

public:
  setFieldOperator(const OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  virtual ~setFieldOperator();

  virtual void addIntoDictionary(OFDictData::dict& setFieldDict) const =0;
};



typedef std::shared_ptr<setFieldOperator> setFieldOperatorPtr;




class fieldToCellOperator
: public setFieldOperator
{
public:
#include "setfields__fieldToCellOperator__Parameters.h"
/*
PARAMETERSET>>> fieldToCellOperator Parameters
inherits setFieldOperator::Parameters

fieldName = string "" "name of the field"
min = double 0.0 "minimum value"
max = double 1e10 "maximum value"

createGetter
<<<PARAMETERSET
*/


public:
  fieldToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  void addIntoDictionary(OFDictData::dict& setFieldDict) const override;
};




class boxToCellOperator
: public setFieldOperator
{
public:
#include "setfields__boxToCellOperator__Parameters.h"
/*
PARAMETERSET>>> boxToCellOperator Parameters
inherits setFieldOperator::Parameters

fieldName = string "" "name of the field"
min = vector (-1e10 -1e10 -1e10) "minimum corner"
max = vector (1e10 1e10 1e10) "maximum corner"

createGetter
<<<PARAMETERSET
*/


public:
  boxToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  void addIntoDictionary(OFDictData::dict& setFieldDict) const override;
};




class cellToCellOperator
: public setFieldOperator
{
public:
#include "setfields__cellToCellOperator__Parameters.h"
/*
PARAMETERSET>>> cellToCellOperator Parameters
inherits setFieldOperator::Parameters

cellSet = string "cellSet" "name of the cell set"

createGetter
<<<PARAMETERSET
*/


public:
  cellToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  virtual void addIntoDictionary(OFDictData::dict& setFieldDict) const;
};




class zoneToCellOperator
: public setFieldOperator
{
public:
#include "setfields__zoneToCellOperator__Parameters.h"
/*
PARAMETERSET>>> zoneToCellOperator Parameters
inherits setFieldOperator::Parameters

cellZone = string "" "name of the cell zone"

createGetter
<<<PARAMETERSET
*/


public:
  zoneToCellOperator(const OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  virtual void addIntoDictionary(OFDictData::dict& setFieldDict) const;
};




}




void setFields(
        const OpenFOAMCase& ofc,
        const boost::filesystem::path& location,
        const setFieldOps::setFieldOperator::Parameters::fieldValues_type& defaultValues,
        const std::vector<setFieldOps::setFieldOperatorPtr>& ops);




#endif


} // namespace insight

#endif // INSIGHT_SETFIELDS_H
