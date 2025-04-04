#include "dynamicclassparametersselectablesubsetgenerator.h"

using namespace std;

defineType(DynamicClassParametersSelectableSubsetGenerator);
addToStaticFunctionTable(ParameterGenerator, DynamicClassParametersSelectableSubsetGenerator, insertrule);




DynamicClassParametersSelectableSubsetGenerator
::DynamicClassParametersSelectableSubsetGenerator (
    const std::string& base,
    const std::string& default_sel,
    const std::string& d )
    : ParameterGenerator ( d ),
    base_type( base ),
    default_sel_(default_sel)
{}



void DynamicClassParametersSelectableSubsetGenerator
    ::cppAddRequiredInclude ( std::set<std::string>& headers ) const
{
  headers.insert("\"base/parameters/selectablesubsetparameter.h\"");
}




std::string
DynamicClassParametersSelectableSubsetGenerator
    ::cppInsightType () const
{
    return "insight::SelectableSubsetParameter";
};




std::string
DynamicClassParametersSelectableSubsetGenerator
    ::cppStaticType () const
{
    return "std::shared_ptr<ParameterSet>";
}




std::string
DynamicClassParametersSelectableSubsetGenerator
    ::cppDefaultValueExpression() const
{
    return "{ \""+default_sel_+"\", "+base_type+"::defaultParametersFor(\""+default_sel_+"\") }";
}




void
DynamicClassParametersSelectableSubsetGenerator
    ::writeCppTypeDecl(
    std::ostream& os ) const
{
    os
        << "struct " <<cppTypeName()<<" "
        << "{\n"
        <<  "std::string selection;\n"
        <<   cppStaticType()<< " parameters;\n"
        << "};\n";
}




void
DynamicClassParametersSelectableSubsetGenerator
    ::cppWriteCreateStatement (
    std::ostream& os,
    const std::string& psvarname ) const
{

    os << "auto "<<psvarname<<" = std::make_unique< "<<cppInsightType ()<<" >("
       <<  cppInsightTypeConstructorParameters()<<");\n"
       << "{\n"
       <<  "for ("<<base_type<<"::FactoryTable::const_iterator i = "<<base_type<<"::factories_->begin();"
            "i != "<<base_type<<"::factories_->end(); i++)"
           "{"
               "auto defp = "<<base_type<<"::defaultParametersFor(i->first);\n"
                <<psvarname<<"->addItem( i->first, std::move(defp) );\n"
           "}";

    if (default_sel_==std::string())
         os<<psvarname<<"->setSelection("<<base_type<<"::factories_->begin()->first);\n";
    else
         os<<psvarname<<"->setSelection(\""<<default_sel_<<"\");\n";

    os << "}\n"
    ;
}




void DynamicClassParametersSelectableSubsetGenerator
    ::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os<<"{\n"
        <<varname<<".setSelection("<<staticname<<".selection);\n"
        <<varname<<"() = *" << staticname<<".parameters;\n"
        "}\n"<<endl;
}




void DynamicClassParametersSelectableSubsetGenerator
    ::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{

    os <<staticname<< ".selection = "<<varname<<".selection();\n"
       <<staticname<< ".parameters = std::dynamic_unique_ptr_cast<insight::ParameterSet>("<<varname<<"().clone(false));\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n";
}
