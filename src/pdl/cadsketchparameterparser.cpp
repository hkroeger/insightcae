#include "cadsketchparameterparser.h"

#include "boost/range/adaptors.hpp"

defineType(CADSketchParameterParser);
addToStaticFunctionTable(ParserDataBase, CADSketchParameterParser, insertrule);


CADSketchParameterParser::Data::Data(
    const std::string& script,
    const std::string& makeDefaultParametersFunctionName,
    const std::string& makeDefaultLayerParametersFunctionName,
    const std::string& sketchAppearanceFunctionName,
    const std::vector<boost::fusion::vector2<int, std::string> >& references,
    const std::string& d)
    : ParserDataBase(d),
    makeDefaultParametersFunctionName_(
          makeDefaultParametersFunctionName),
    makeDefaultLayerParametersFunctionName_(
        makeDefaultLayerParametersFunctionName),
    sketchAppearanceFunctionName_(
        sketchAppearanceFunctionName),
    script_(script),
    references_(references)
{}

std::string CADSketchParameterParser::Data::refParameter() const
{
    std::string r="{";
    for (auto ref: boost::adaptors::index(references_))
    {
        if (ref.index()>0) r+=", ";
        r+=str(boost::format("{%d, \"%s\"}")
                 % boost::fusion::get<0>(ref.value())
                 % boost::fusion::get<1>(ref.value()) );
    }
    r+="}";
    return r;
}

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

    std::string mdlpn(makeDefaultLayerParametersFunctionName_);
    if (mdlpn.empty()) mdlpn="[]() { return insight::ParameterSet(); }";

    std::string san(sketchAppearanceFunctionName_);
    if (san.empty()) san="[](const insight::ParameterSet&, vtkProperty*) {}";

    return
        "\""+ script_ + "\", " +
           mdpn + ", " +
           mdlpn + ", " +
           san + ", " +
           refParameter();
}

std::string CADSketchParameterParser::Data::cppConstructorParameters(
    const std::string &name,
    const std::string& thisscope ) const
{
    std::string mdpn(makeDefaultParametersFunctionName_);
    if (mdpn.empty()) mdpn="[]() { return insight::ParameterSet(); }";

    std::string mdlpn(makeDefaultLayerParametersFunctionName_);
    if (mdlpn.empty()) mdlpn="[]() { return insight::ParameterSet(); }";

    std::string san(sketchAppearanceFunctionName_);
    if (san.empty()) san="[](const insight::ParameterSet&, vtkProperty*) {}";

    return cppType(name)+"(new "
           + cppParamType(name)
           +"( \"" + script_ + "\", "
           + mdpn + ", "
           + mdlpn + ", "
           + san + ", "
           + refParameter() + ", "
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
    os<<staticname<<".reset( "<<varname<<".cloneCADSketchParameter(true) );"<<std::endl;
}

