#include "directorygenerator.h"


defineType(DirectoryGenerator);
addToStaticFunctionTable(ParameterGenerator, DirectoryGenerator, insertrule);

DirectoryGenerator::DirectoryGenerator(const boost::filesystem::path& v, const std::string& d)
    : ParameterGenerator(d), value(v)
{}

void DirectoryGenerator::cppAddRequiredInclude(std::set< std::string >& headers) const
{
    headers.insert("<memory>");
    headers.insert("\"base/parameters/pathparameter.h\"");
}

std::string DirectoryGenerator::cppInsightType() const
{
    return "insight::DirectoryParameter";
}

std::string DirectoryGenerator::cppStaticType() const
{
    return "std::shared_ptr<insight::DirectoryParameter>";
}

std::string DirectoryGenerator::cppDefaultValueExpression() const
{
    return "\""+value.string()+"\"";
}

std::string DirectoryGenerator::cppConstructorParameters() const
{
    return cppStaticType()+"(new "
           + cppInsightType()
           +"( \""
           + value.string() + "\", \""
           +cppInsightTypeConstructorParameters()+"))";
}




/**
 * write the code to
 * transfer the values form the static c++ struct into the dynamic parameter set
 */
void DirectoryGenerator::cppWriteSetStatement
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
void DirectoryGenerator::cppWriteGetStatement
    (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
        ) const
{
    os <<staticname<< "=std::move( "<<varname<<".cloneDirectoryParameter() );\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n";
}
