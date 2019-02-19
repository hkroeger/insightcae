#include "arrayparameterparser.h"

using namespace std;

ArrayParameterParser::Data::Data(ParserDataBase::Ptr v, size_t n, const std::string& d)
: ParserDataBase(d), value(v), num(n)
{}

void ArrayParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
  headers.insert("<vector>");
  value->cppAddHeader(headers);
}

std::string ArrayParameterParser::Data::cppValueRep(const std::string& name) const
{
  std::ostringstream rep;
  rep << "{";
  for (size_t i=0; i<num; i++)
  {
    if (i>0) rep<<",";
    rep << value->cppValueRep(name+"_default") ;
  }
  rep << "}";
  return rep.str();
}

std::string ArrayParameterParser::Data::cppType(const std::string& name) const
{
  return "std::vector<"+value->cppTypeName(name+"_default")+">";
}

std::string ArrayParameterParser::Data::cppTypeDecl ( const std::string& name ) const
{
  return
    value->cppTypeDecl ( name+"_default" )
    +"\n"
    +ParserDataBase::cppTypeDecl ( name );
}

std::string ArrayParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::ArrayParameter";
}

void ArrayParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name
) const
{

  os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<"(new "<<cppParamType(name)<<"(\""<<description<<"\")); "<<endl;

  os<<"{"<<endl;
  value->cppWriteCreateStatement
  (
    os, name+"_default_value"
  );
  os<<name<<"->setDefaultValue(*"<<name<<"_default_value.release());"<<endl;
  if (num>0)
  {
    os<<"for (size_t i=0; i<"<<num<<"; i++) "<<name<<"->appendEmpty();"<<endl;
  }
  os<<"}"<<endl;
}

void ArrayParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& varname,
    const std::string& staticname,
    const std::string& thisscope
) const
{
  os<<varname<<".clear();"<<endl;
  os<<"for(size_t k=0; k<"<<staticname<<".size(); k++)"<<endl;
  os<<"{"<<endl;
  os<<varname<<".appendEmpty();"<<endl;

  os<<value->cppParamType(name+"_default")<<"& "<<varname
    <<"_cur = dynamic_cast< "<< value->cppParamType(name+"_default") <<"& >("<<varname<<"[k]);"<<endl;
  os<<"const "<<extendtype(thisscope, value->cppTypeName(name+"_default"))<<"& "<<varname<<"_cur_static = "<<staticname<<"[k];"<<endl;

  value->cppWriteSetStatement(os, name+"_default", name+"_cur", name+"_cur_static", thisscope);

  os<<"}"<<endl;
}

void ArrayParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& varname,
    const std::string& staticname,
    const std::string& thisscope
) const
{
  os<<staticname<<".resize("<<varname<<".size());"<<endl;
  os<<"for(int k=0; k<"<<varname<<".size(); k++)"<<endl;
  os<<"{"<<endl;
  os<<"const "<<value->cppParamType(name+"_default")<<"& "<<varname
    <<"_cur = dynamic_cast<const "<< value->cppParamType(name+"_default") <<"& >("<<varname<<"[k]);"<<endl;
  os<<extendtype(thisscope, value->cppTypeName(name+"_default"))<<"& "<<varname<<"_cur_static = "<<staticname<<"[k];"<<endl;

  value->cppWriteGetStatement(os, name+"_default", name+"_cur", name+"_cur_static", thisscope);

  os<<"}"<<endl;
}

