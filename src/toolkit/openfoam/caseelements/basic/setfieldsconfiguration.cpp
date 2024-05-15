#include "setfieldsconfiguration.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"


namespace insight {

defineType(setFieldsConfiguration);
addToOpenFOAMCaseElementFactoryTable(setFieldsConfiguration);



setFieldsConfiguration::setFieldsConfiguration( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "setFieldsConfiguration", ps),
  p_(ps)
{
}


void setFieldsConfiguration::addIntoDictionaries(OFdicts& dictionaries) const
{

    OFDictData::dict& sFD
      = dictionaries.lookupDict("system/setFieldsDict");

    OFDictData::list dfvs;
    for (const Parameters::defaultValues_default_type& dfv: p_.defaultValues)
    {
      if (const auto * sdfv = boost::get<Parameters::defaultValues_default_scalar_type>(&dfv))
        {
          dfvs.push_back(boost::str(boost::format("volScalarFieldValue %s %g\n") % sdfv->name % sdfv->value));
        }
      else if (const auto * vdfv = boost::get<Parameters::defaultValues_default_vector_type>(&dfv))
        {
          dfvs.push_back(boost::str(boost::format("volVectorFieldValue %s %s\n") % vdfv->name % OFDictData::to_OF(vdfv->value)));
        }
      else throw insight::UnhandledSelection();
    }
    sFD["defaultFieldValues"]=dfvs;

    OFDictData::list rs;
    for (const Parameters::regionSelectors_default_type& r: p_.regionSelectors)
    {
      if (const auto * box = boost::get<Parameters::regionSelectors_default_box_type>(&r))
        {
          OFDictData::list vl;
          for (const Parameters::regionSelectors_default_box_type::regionValues_default_type& bv: box->regionValues)
          {
            if (const auto * s = boost::get<Parameters::regionSelectors_default_box_type::regionValues_default_scalar_type>(&bv))
              {
                vl.push_back(boost::str(boost::format("volScalarFieldValue %s %g\n") % s->name % s->value));
              }
            else if (const auto * v = boost::get<Parameters::regionSelectors_default_box_type::regionValues_default_vector_type>(&bv))
              {
                vl.push_back(boost::str(boost::format("volVectorFieldValue %s %s\n") % v->name % OFDictData::to_OF(v->value)));
              }
            else throw insight::UnhandledSelection();

            OFDictData::dict fs;
            fs["box"]=boost::str(boost::format("%s %s") % OFDictData::to_OF(box->p0) % OFDictData::to_OF(box->p1) );
            fs["fieldValues"]=vl;

            if (box->selectcells)
              {
                rs.push_back("boxToCell");
                rs.push_back(fs);
              }

            if (box->selectfaces)
              {
                rs.push_back("boxToFace");
                rs.push_back(fs);
              }
          }
        }
      else
        throw insight::Exception("Internal error: Unhandled region selection!");
    }
    sFD["regions"]=rs;
}


bool setFieldsConfiguration::isUnique() const
{
  return true;
}


} // namespace insight
