#include "dynamicclassparametersselectablesubsetparameterparser.h"

using namespace std;

DynamicClassParametersSelectableSubsetParameterParser::Data::Data ( const std::string& base,  const std::string& default_sel, const std::string& d )
    : ParserDataBase ( d ), base_type( base ), default_sel_(default_sel)
{}

void DynamicClassParametersSelectableSubsetParameterParser::Data::cppAddHeader ( std::set<std::string>&  ) const
{
}

std::string DynamicClassParametersSelectableSubsetParameterParser::Data::cppType ( const std::string&  ) const
{
    std::ostringstream os;
    os << "ParameterSet";
    return os.str();
}

std::string DynamicClassParametersSelectableSubsetParameterParser::Data::cppTypeDecl(const std::string& name) const
{
    return std::string("struct "+cppTypeName(name)+" { std::string selection; ")+cppType(name)+" parameters; };";
}


std::string DynamicClassParametersSelectableSubsetParameterParser::Data::cppValueRep ( const std::string&  ) const
{
  return "{ \""+default_sel_+"\", "+base_type+"::defaultParameters(\""+default_sel_+"\") }";
}


std::string DynamicClassParametersSelectableSubsetParameterParser::Data::cppParamType ( const std::string&  ) const
{
    return "insight::SelectableSubsetParameter";
};

void DynamicClassParametersSelectableSubsetParameterParser::Data::cppWriteCreateStatement ( std::ostream& os, const std::string& name ) const
{

    os <<
         "std::auto_ptr< "<<cppParamType ( name ) <<" > "<<name<<";"
         "{"
         <<name<<".reset(new "<<cppParamType ( name ) <<"(\""<<description<<"\")); "
        "for ("<<base_type<<"::FactoryTable::const_iterator i = "<<base_type<<"::factories_->begin();"
            "i != "<<base_type<<"::factories_->end(); i++)"
        "{"
            "ParameterSet defp = "<<base_type<<"::defaultParameters(i->first);\n"
            <<name<<"->addItem( i->first, defp );\n"
        "}";

    if (default_sel_==std::string())
         os<<name<<"->selection() = "<<base_type<<"::factories_->begin()->first;\n";
    else
         os<<name<<"->selection() = \""<<default_sel_<<"\";\n";
    os << "}"
    ;
}

void DynamicClassParametersSelectableSubsetParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& ,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<"{\n"
        <<varname<<".selection()="<<staticname<<".selection;\n"
        <<varname<<"() = " << staticname<<".parameters;\n"
        "}\n"<<endl;
}

void DynamicClassParametersSelectableSubsetParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& ,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{

    os<<
    "{\n"
      <<staticname<<".selection = "<<varname<<".selection();\n"
      <<staticname<<".parameters = "<<varname<<"();\n"
    "}\n";
}
