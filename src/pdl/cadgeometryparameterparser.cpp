#include "cadgeometryparameterparser.h"


CADGeometryParameterParser::Data::Data(
    const std::string& featureLabel,
    const std::string& script,
    const std::string& d)
    : ParserDataBase(d), featureLabel_(featureLabel), script_(script)
{}

void CADGeometryParameterParser::Data::cppAddHeader(std::set< std::string >& headers) const
{
  headers.insert("<memory>");
  headers.insert("\"cadgeometryparameter.h\"");
}

std::string CADGeometryParameterParser::Data::cppType(const std::string&) const
{
  return "std::shared_ptr<insight::CADGeometryParameter>";
}

std::string CADGeometryParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::CADGeometryParameter";
}

std::string CADGeometryParameterParser::Data::cppValueRep(const std::string& name, const std::string& thisscope) const
{
  return "\""+featureLabel_+"\", \""+ script_ + "\"";
}

std::string CADGeometryParameterParser::Data::cppConstructorParameters(const std::string &name,
                                                                const std::string& thisscope) const
{
  return cppType(name)+"(new "
    + cppParamType(name)
    +"( \"" + featureLabel_ + "\", "
       "\"" + script_ + "\", "
       "\"" + description + "\", "
      + (isHidden?"true":"false")+","
      + (isExpert?"true":"false")+","
      + (isNecessary?"true":"false")+","
      +boost::lexical_cast<std::string>(order)
    +"))";
}




/**
 * write the code to
 * transfer the values form the static c++ struct into the dynamic parameter set
 */
void CADGeometryParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<varname<<" = *"<<staticname<<";"<<std::endl;
}

/**
 * write the code to
 * transfer values from the dynamic parameter set into the static c++ data structure
 */
void CADGeometryParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<staticname<<".reset( "<<varname<<".cloneCADGeometryParameter() );"<<std::endl;
}

