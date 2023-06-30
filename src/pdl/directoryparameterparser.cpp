#include "directoryparameterparser.h"


defineType(DirectoryParameterParser);
addToStaticFunctionTable(ParserDataBase, DirectoryParameterParser, insertrule);

DirectoryParameterParser::Data::Data(const boost::filesystem::path& v, const std::string& d)
    : ParserDataBase(d), value(v)
{}

void DirectoryParameterParser::Data::cppAddHeader(std::set< std::string >& headers) const
{
    headers.insert("<memory>");
    headers.insert("\"base/parameters/pathparameter.h\"");
}

std::string DirectoryParameterParser::Data::cppType(const std::string&) const
{
    return "std::shared_ptr<insight::DirectoryParameter>";
}

std::string DirectoryParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::DirectoryParameter";
}

std::string DirectoryParameterParser::Data::cppValueRep(const std::string& name, const std::string& thisscope) const
{
    return "\""+value.string()+"\"";
}

std::string DirectoryParameterParser::Data::cppConstructorParameters(const std::string &name,
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
void DirectoryParameterParser::Data::cppWriteSetStatement
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
void DirectoryParameterParser::Data::cppWriteGetStatement
    (
        std::ostream& os,
        const std::string&,
        const std::string& varname,
        const std::string& staticname,
        const std::string&
        ) const
{
    os<<staticname<<".reset( "<<varname<<".cloneDirectoryParameter() );"<<std::endl;
}
