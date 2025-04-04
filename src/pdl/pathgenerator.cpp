#include "pathgenerator.h"

defineType(PathGenerator);
addToStaticFunctionTable(ParameterGenerator, PathGenerator, insertrule);

PathGenerator::PathGenerator(
    const boost::filesystem::path& v, const std::string& d)
: ParameterGenerator(d), value(v)
{}

void PathGenerator::cppAddRequiredInclude(std::set< std::string >& headers) const
{
  headers.insert("<memory>");
  headers.insert("\"base/parameters/pathparameter.h\"");
}

std::string PathGenerator::cppInsightType() const
{
    return "insight::PathParameter";
}


std::string PathGenerator::cppStaticType() const
{
  return "std::shared_ptr<insight::PathParameter>";
}


std::string PathGenerator::cppDefaultValueExpression() const
{
  return "\""+value.string()+"\"";
}

std::string PathGenerator::cppConstructorParameters() const
{
  return cppStaticType()+"(new "
    + cppInsightType()
    +"( \""
      + value.string() + "\", \""
      + cppInsightTypeConstructorParameters() +"))";
}




/**
 * write the code to
 * transfer the values form the static c++ struct into the dynamic parameter set
 */
void PathGenerator::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os<<varname<<" = *"<<staticname<<";"<<std::endl;
}

/**
 * write the code to
 * transfer values from the dynamic parameter set into the static c++ data structure
 */
void PathGenerator::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os <<staticname<< "=std::move("<<varname<<".clonePathParameter());\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n" ;
}
