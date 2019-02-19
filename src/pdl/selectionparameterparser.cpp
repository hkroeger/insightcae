#include "selectionparameterparser.h"


using namespace std;

SelectionParameterParser::Data::Data(const std::vector<std::string>& sels, const std::string& sel, const std::string& d)
: ParserDataBase(d), selections(sels), selection(sel)
{}

std::string SelectionParameterParser::Data::cppType(const std::string&) const
{
  return "#error";
}

std::string SelectionParameterParser::Data::cppTypeDecl(const std::string& name) const
{
  std::ostringstream os;
  os<<"enum "<<cppTypeName(name)<<"\n{"<<endl;
  std::string comma="";
  for (const std::string& s: selections)
  {
    os<<comma<<s<<endl;
    comma=",";
  }
  os<<"};";
  return os.str();
}
std::string SelectionParameterParser::Data::cppValueRep(const std::string& ) const
{
  return selection;
}

std::string SelectionParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::SelectionParameter";
}

void SelectionParameterParser::Data::cppWriteCreateStatement(std::ostream& os, const std::string& name) const
{

  os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<";"<<endl;
//       os<<cppParamType(name)<<"& "<<s_fq_name <<" = *value;"<<endl;
  os<<"{"<<endl;
  os<<"insight::SelectionParameter::ItemList items;"<<endl;
  for (const std::string& s: selections)
  {
    os<<"items.push_back(\""<<s<<"\");"<<endl;
  }
  os<<name<<".reset(new "<<cppParamType(name)<<"(\""<< selection <<"\", items, \""<<description<<"\")); "<<endl;
  os<<"}"<<endl;
}

void SelectionParameterParser::Data::cppWriteSetStatement(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
  os<<varname<<"() = int("<< staticname <<");"<<endl;
}

void SelectionParameterParser::Data::cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
  const std::string& thisscope) const
{
  os<<staticname<<"="<<extendtype(thisscope, cppTypeName(name))<<"("<<varname<<"());"<<endl;
}
