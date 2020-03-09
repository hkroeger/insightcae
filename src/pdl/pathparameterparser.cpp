#include "pathparameterparser.h"


PathParameterParser::Data::Data(const boost::filesystem::path& v, const std::string& d)
: ParserDataBase(d), value(v)
{}

void PathParameterParser::Data::cppAddHeader(std::set< std::string >& headers) const
{
//  headers.insert("<boost/filesystem.hpp>");
  headers.insert("<memory>");
}

std::string PathParameterParser::Data::cppType(const std::string&) const
{
//  return "boost::filesystem::path";
  return "std::shared_ptr<insight::PathParameter>";
}

std::string PathParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::PathParameter";
}

std::string PathParameterParser::Data::cppValueRep(const std::string& name, const std::string& thisscope) const
{
  return "\""+value.string()+"\"";
}

std::string PathParameterParser::Data::cppConstructorParameters(const std::string &name,
                                                                const std::string& thisscope) const
{
  return cppType(name)+"(new "
    + cppParamType(name)
    +"( \""
      + value.string() + "\", \""
      + description + "\", "
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
void PathParameterParser::Data::cppWriteSetStatement
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
void PathParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<staticname<<".reset( "<<varname<<".clonePathParameter() );"<<std::endl;
}
