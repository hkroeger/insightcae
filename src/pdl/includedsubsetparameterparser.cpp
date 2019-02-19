#include "includedsubsetparameterparser.h"

using namespace std;


IncludedSubsetParameterParser::Data::Data(const std::string& v, const std::string& d)
    : ParserDataBase(d), value(v)
{}

std::string IncludedSubsetParameterParser::Data::cppType(const std::string&) const
{
    return value;
}

std::string IncludedSubsetParameterParser::Data::cppValueRep(const std::string& ) const
{
    return "#error";
}

std::string IncludedSubsetParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::SubsetParameter";
}

void IncludedSubsetParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name
) const
{
    os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name
    <<"(new "<<cppParamType(name)<<"("<< value <<"::makeDefault(), \""<<description<<"\")); "<<endl;
}

void IncludedSubsetParameterParser::Data::cppWriteInsertStatement
(
    std::ostream& os,
    const std::string& psvarname,
    const std::string& name
) const
{
    os<<"{ ";
    os<<"std::string key(\""<<name<<"\"); ";
    this->cppWriteCreateStatement(os, name);
    os<<"if ("<<psvarname<<".find(key)!="<<psvarname<<".end()) {"<<endl;
    os<<psvarname<<".getSubset(key).merge(*"<<name<<"); ";
    os<<"} else {"<<endl;
    os<<psvarname<<".insert(key, "<<name<<"); ";
    os<<"}"<<endl;
    os<<"}"<<endl;
}


void IncludedSubsetParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& ,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os << staticname << ".set(" << varname << ");" << endl;
}

void IncludedSubsetParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& ,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os << staticname << ".get(" << varname << ");" << endl;
}
