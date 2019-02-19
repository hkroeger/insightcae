#include "dynamicclassselectablesubsetparameterparser.h"

using namespace std;

DynamicClassSelectableSubsetParameterParser::Data::Data ( const std::string& base,  const std::string& default_sel, const std::string& d )
    : ParserDataBase ( d ), base_type( base ), default_sel_(default_sel)
{}

void DynamicClassSelectableSubsetParameterParser::Data::cppAddHeader ( std::set<std::string>&  ) const
{
}

std::string DynamicClassSelectableSubsetParameterParser::Data::cppType ( const std::string&  ) const
{
    std::ostringstream os;
    os << "std::shared_ptr<"<<base_type<<">";
    return os.str();
}
std::string DynamicClassSelectableSubsetParameterParser::Data::cppValueRep ( const std::string&  ) const
{
    return "#error";
}


std::string DynamicClassSelectableSubsetParameterParser::Data::cppParamType ( const std::string&  ) const
{
    return "insight::SelectableSubsetParameter";
}

void DynamicClassSelectableSubsetParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name
) const
{

    os <<
         "std::auto_ptr< "<<cppParamType ( name ) <<" > "<<name<<";"
         "{"
         <<name<<".reset(new "<<cppParamType ( name ) <<"(\""<<description<<"\")); "
        "for ("<<base_type<<"::FactoryTable::const_iterator i = "<<base_type<<"::factories_->begin();"
            "i != "<<base_type<<"::factories_->end(); i++)"
        "{"
            "ParameterSet defp = "<<base_type<<"::defaultParameters(i->first);"
            <<name<<"->addItem( i->first, defp );"
        "}";

    if (default_sel_==std::string())
         os<<name<<"->selection() = "<<base_type<<"::factories_->begin()->first;";
    else
         os<<name<<"->selection() = \""<<default_sel_<<"\";";

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
          <<varname<<".selection()="<<staticname<<"->type();"
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
