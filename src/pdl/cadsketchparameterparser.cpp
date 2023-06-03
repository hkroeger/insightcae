#include "cadsketchparameterparser.h"

defineType(CADSketchParameterParser);
addToStaticFunctionTable(ParserDataBase, CADSketchParameterParser, insertrule);


CADSketchParameterParser::Data::Data(
    const std::string& script,
    const std::string& makeDefaultParametersFunctionName,
    const std::string& d)
    : ParserDataBase(d), makeDefaultParametersFunctionName_(makeDefaultParametersFunctionName), script_(script)
{}

void CADSketchParameterParser::Data::cppAddHeader(std::set< std::string >& headers) const
{
    headers.insert("<memory>");
    headers.insert("\"cadsketchparameter.h\"");
}

std::string CADSketchParameterParser::Data::cppType(const std::string&) const
{
    return "std::shared_ptr<insight::CADSketchParameter>";
}

std::string CADSketchParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::CADSketchParameter";
}

std::string CADSketchParameterParser::Data::cppValueRep(const std::string& name, const std::string& thisscope) const
{
    std::string mdpn(makeDefaultParametersFunctionName_);
    if (mdpn.empty()) mdpn="[]() { return insight::ParameterSet(); }";

    return "\""+ script_ + "\", " + mdpn;
}

std::string CADSketchParameterParser::Data::cppConstructorParameters(const std::string &name,
                                                                       const std::string& thisscope) const
{
    std::string mdpn(makeDefaultParametersFunctionName_);
    if (mdpn.empty()) mdpn="[]() { return insight::ParameterSet(); }";

    return cppType(name)+"(new "
           + cppParamType(name)
           +"( \"" + script_ + "\", "
           + mdpn + ", "
           + "\"" + description + "\", "
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
void CADSketchParameterParser::Data::cppWriteSetStatement
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
void CADSketchParameterParser::Data::cppWriteGetStatement
    (
        std::ostream& os,
        const std::string&,
        const std::string& varname,
        const std::string& staticname,
        const std::string&
        ) const
{
    os<<staticname<<".reset( "<<varname<<".cloneCADSketchParameter() );"<<std::endl;
}

