#include "cadgeometryparameterparser.h"


defineType(CADGeometryGenerator);
addToStaticFunctionTable(ParameterGenerator, CADGeometryGenerator, insertrule);



CADGeometryGenerator::CADGeometryGenerator(
    const std::string& featureLabel,
    const std::string& script,
    const std::string& d)
    : ParameterGenerator(d), featureLabel_(featureLabel), script_(script)
{}

void CADGeometryGenerator::cppAddRequiredInclude(std::set< std::string >& headers) const
{
  headers.insert("<memory>");
  headers.insert("\"cadgeometryparameter.h\"");
}

std::string CADGeometryGenerator::cppInsightType() const
{
    return "insight::CADGeometryParameter";
}

std::string CADGeometryGenerator::cppStaticType() const
{
  return "std::shared_ptr<insight::CADGeometryParameter>";
}

std::string CADGeometryGenerator::cppDefaultValueExpression() const
{
  return "\""+featureLabel_+"\", \""+ script_ + "\"";
}

std::string CADGeometryGenerator::cppConstructorParameters() const
{
  return cppStaticType()+"(new "
    + cppInsightType()
    +"( \"" + featureLabel_ + "\", "
       "\"" + script_ + "\", "
           +cppInsightTypeConstructorParameters()
    +"))";
}




/**
 * write the code to
 * transfer the values form the static c++ struct into the dynamic parameter set
 */
void CADGeometryGenerator::cppWriteSetStatement
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
void CADGeometryGenerator::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os
        <<staticname<< ".reset( "<<varname<<".cloneCADGeometryParameter() );\n"
        <<staticname<< ".setPath( "<<varname<<" .path());\n";
}

