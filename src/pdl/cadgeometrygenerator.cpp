#include "cadgeometrygenerator.h"


defineType(CADGeometryGenerator);
addToStaticFunctionTable(ParameterGenerator, CADGeometryGenerator, insertrule);

CADGeometryGenerator::CADGeometryGenerator(
    const boost::filesystem::path& v, const std::string& d)
    : ParameterGenerator(d), value(v)
{}

void CADGeometryGenerator::cppAddRequiredInclude(std::set< std::string >& headers) const
{
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
    return "boost::filesystem::path(\""+value.string()+"\")";
}

std::string CADGeometryGenerator::cppConstructorParameters() const
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
void CADGeometryGenerator::cppWriteSetStatement
    (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
        ) const
{
    os<<varname<<".assignFrom(*"<<staticname<<");"<<std::endl;
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
    os <<staticname<< "=std::move("<<varname<<".cloneAs<insight::CADGeometryParameter>());\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n" ;
}
