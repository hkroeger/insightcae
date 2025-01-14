#include "dynamicclassselectablesubsetparameterparser.h"

using namespace std;


defineType(DynamicClassSelectableSubsetParameterParser);
addToStaticFunctionTable(
    ParameterGenerator,
    DynamicClassSelectableSubsetParameterParser, insertrule);

DynamicClassSelectableSubsetParameterParser
    ::DynamicClassSelectableSubsetParameterParser (
    const std::string& base,  const std::string& default_sel, const std::string& d )
    : ParameterGenerator ( d ), base_type( base ), default_sel_(default_sel)
{}


void DynamicClassSelectableSubsetParameterParser
    ::cppAddRequiredInclude (
    std::set<std::string>& headers ) const
{
  headers.insert("\"base/parameters/selectablesubsetparameter.h\"");
}


std::string
DynamicClassSelectableSubsetParameterParser
    ::cppInsightType () const
{
    return "insight::SelectableSubsetParameter";
}


std::string
DynamicClassSelectableSubsetParameterParser
    ::cppStaticType () const
{
    std::ostringstream os;
    os << "std::shared_ptr<"<<base_type<<">";
    return os.str();
}


std::string
DynamicClassSelectableSubsetParameterParser
    ::cppDefaultValueExpression() const
{
    return "nullptr";
}



void DynamicClassSelectableSubsetParameterParser
    ::cppWriteCreateStatement(
    std::ostream& os,
    const std::string& psvarname ) const
{

    os <<
         "std::unique_ptr< "<<cppInsightType () <<" > "<<psvarname<<";"
         "{"
         <<psvarname<<".reset(new "<<cppInsightType () <<"(\""<<description<<"\")); "
        "for (auto i = "<<base_type<<"::factories_->begin();"
            "i != "<<base_type<<"::factories_->end(); i++)"
        "{"
            "auto defp = "<<base_type<<"::defaultParametersFor(i->first);"
            <<psvarname<<"->addItem( i->first, std::move(defp) );"
        "}";

    if (default_sel_==std::string())
         os<<psvarname<<"->setSelection("<<base_type<<"::factories_->begin()->first);";
    else
         os<<psvarname<<"->setSelection(\""<<default_sel_<<"\");";

    os << "}"
    ;
}

void DynamicClassSelectableSubsetParameterParser
    ::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os<<"{"
          <<varname<<".setSelection("<<staticname<<"->type());"
          <<varname<<"() = "<<staticname<<"->getParameters();"
        "}"<<endl;
}

void DynamicClassSelectableSubsetParameterParser
    ::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{

    os<<
    "{"

        "std::string typ = "<<varname<<".selection();\n"
        "auto& param = "<<varname<<"();\n"
        <<staticname<< ".reset( " <<base_type<<"::lookup( typ, param) );\n"
        <<staticname<< ".setPath( "<<varname<<" .path());\n"
    "}";
}
