#include "dynamicclassselectablesubsetparameterparser.h"

using namespace std;


defineType(DynamicClassSelectableSubsetParameterParser);
addToStaticFunctionTable(ParserDataBase, DynamicClassSelectableSubsetParameterParser, insertrule);

DynamicClassSelectableSubsetParameterParser::Data::Data ( const std::string& base,  const std::string& default_sel, const std::string& d )
    : ParserDataBase ( d ), base_type( base ), default_sel_(default_sel)
{}

void DynamicClassSelectableSubsetParameterParser::Data::cppAddHeader ( std::set<std::string>& headers ) const
{
  headers.insert("\"base/parameters/selectablesubsetparameter.h\"");
}

std::string DynamicClassSelectableSubsetParameterParser::Data::cppType ( const std::string&  ) const
{
    std::ostringstream os;
    os << "std::shared_ptr<"<<base_type<<">";
    return os.str();
}
std::string DynamicClassSelectableSubsetParameterParser::Data::cppValueRep ( const std::string&, const std::string& thisscope  ) const
{
    return "nullptr";
}


std::string DynamicClassSelectableSubsetParameterParser::Data::cppParamType ( const std::string&  ) const
{
    return "insight::SelectableSubsetParameter";
}

void DynamicClassSelectableSubsetParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& thisscope
) const
{

    os <<
         "std::unique_ptr< "<<cppParamType ( name ) <<" > "<<name<<";"
         "{"
         <<name<<".reset(new "<<cppParamType ( name ) <<"(\""<<description<<"\")); "
        "for (auto i = "<<base_type<<"::factories_->begin();"
            "i != "<<base_type<<"::factories_->end(); i++)"
        "{"
            "ParameterSet defp = "<<base_type<<"::defaultParametersFor(i->first);"
            <<name<<"->addItem( i->first, defp );"
        "}";

    if (default_sel_==std::string())
         os<<name<<"->setSelection("<<base_type<<"::factories_->begin()->first);";
    else
         os<<name<<"->setSelection(\""<<default_sel_<<"\");";

    os << "}"
    ;
}

void DynamicClassSelectableSubsetParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& ,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<"{"
          <<varname<<".setSelection("<<staticname<<"->type());"
          <<varname<<"()="<<staticname<<"->getParameters();"
        "}"<<endl;
}

void DynamicClassSelectableSubsetParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& ,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{

    os<<
    "{"

        "std::string typ = "<<varname<<".selection();"
        "const ParameterSet& param = "<<varname<<"();"

        <<staticname<<".reset ( "<<base_type<<"::lookup( typ, param) );"

    "}";
}
