#include "openfoamcasedirectorygenerator.h"


defineType(OpenFOAMCaseDirectoryGenerator);
addToStaticFunctionTable(ParameterGenerator, OpenFOAMCaseDirectoryGenerator, insertrule);

OpenFOAMCaseDirectoryGenerator::OpenFOAMCaseDirectoryGenerator(const boost::filesystem::path& v, const std::string& d)
    : DirectoryGenerator(v, d)
{}

void OpenFOAMCaseDirectoryGenerator::cppAddRequiredInclude(std::set< std::string >& headers) const
{
    headers.insert("<memory>");
    headers.insert("\"openfoam/openfoamcasedirectoryparameter.h\"");
}

std::string OpenFOAMCaseDirectoryGenerator::cppInsightType() const
{
    return "insight::OpenFOAMCaseDirectoryParameter";
}

std::string OpenFOAMCaseDirectoryGenerator::cppStaticType() const
{
    return "std::shared_ptr<insight::OpenFOAMCaseDirectoryParameter>";
}


/**
 * write the code to
 * transfer values from the dynamic parameter set into the static c++ data structure
 */
void OpenFOAMCaseDirectoryGenerator::cppWriteGetStatement
    (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
        ) const
{
    os <<staticname<< "=std::move( "<<varname<<".cloneOpenFOAMCaseDirectoryParameter() );\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n";
}
