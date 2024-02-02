#include "subsetparameterparser.h"

using namespace std;



defineType(SubsetParameterParser);
addToStaticFunctionTable(ParserDataBase, SubsetParameterParser, insertrule);


SubsetParameterParser::Data::Data(const ParameterSetData& v, const std::string& d)
    : ParserDataBase(d), value(v)
{
}

void SubsetParameterParser::Data::cppAddHeader(std::set< std::string >& headers) const
{
    headers.insert("\"base/parameters/subsetparameter.h\"");

    for (const ParameterSetEntry& pe: value)
    {
        pe.second->cppAddHeader(headers);
    }
}

std::string SubsetParameterParser::Data::cppType(const std::string&) const
{
    return "";
}

std::string SubsetParameterParser::Data::cppTypeDecl(const std::string& name,
                                                     const std::string& thisscope) const
{
    std::ostringstream os;
    std::string structname = cppTypeName(name);
    std::string subscope = extendtype(thisscope, name+"_type");

    os<<"struct "<<structname<<"\n{"<<endl;
    // members
    for (const ParameterSetEntry& pe: value)
    {
                                      /* name */
        pe.second->writeCppHeader( os, pe.first, subscope );
    }

    // default constructor
    os << structname << "()\n";
    if (value.size()>0)
    {
      os<<":";
      bool first=true;
      for (const ParameterSetEntry& pe: value)
      {
          if (!first) os<<",\n"; else first=false;
          os << pe.first<<"("<<(pe.second->cppConstructorParameters(pe.first, subscope))<<")";
      }
    }
    os <<"{}\n";

    if (value.size()>0)
    {
    // explicit constructor
      os << structname << "(";
      {
        bool first=true;
        for (const ParameterSetEntry& pe: value)
        {
            if (!first) os<<",\n"; else first=false;
            os << "const "<<pe.second->cppTypeName(pe.first)<<"& "<<(pe.first+"_value");
        }
      }
      os << ") : \n";
      {
        bool first=true;
        for (const ParameterSetEntry& pe: value)
        {
            if (!first) os<<",\n"; else first=false;
            os << pe.first<<"("<<(pe.first+"_value")<<")";
        }
      }
      os <<"{}\n";
    }

    os<<"};";
    return os.str();
}

std::string SubsetParameterParser::Data::cppValueRep(const std::string& name, const std::string& thisscope ) const
{
    //return "#error";
  std::ostringstream rep;
  if (value.size()>0)
  {
    rep<<"{";
    bool first=true;
    for (const ParameterSetEntry& pe: value)
    {
        if (!first) rep<<",\n"; else first=false;
        rep <<(pe.second->cppConstructorParameters(pe.first, extendtype(thisscope, name+"_type")));
    }
    rep<<"}";
  }
  return rep.str();
}

std::string SubsetParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::SubsetParameter";
}

void SubsetParameterParser::Data::cppWriteInsertStatement
(
    std::ostream& os,
    const std::string& psvarname,
    const std::string& name,
    const std::string& thisscope
) const
{
    os<<"{ ";
    os<<"std::string key(\""<<name<<"\"); ";
    this->cppWriteCreateStatement(os, name, extendtype(thisscope, name+"_type"));
    os<<"if ("<<psvarname<<".contains(key)) {"<<endl;
    os<<psvarname<<".getSubset(key).extend(*"<<name<<");\n";
    os<<psvarname<<".getSubset(key).merge(*"<<name<<");\n";
    os<<"} else {"<<endl;
    os<<psvarname<<".insert(key, std::move("<<name<<")); ";
    os<<"}"<<endl;
    os<<"}"<<endl;
}

void SubsetParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& thisscope
) const
{
    os<<"std::unique_ptr< "<<cppParamType(name)<<" > "<<name<<"(new "<<cppParamType(name)<<"(\""<<description<<"\")); "<<endl;
    os<<"{"<<endl;
    for (const ParameterSetEntry& pe: value)
    {
        pe.second->cppWriteInsertStatement
        (
            os,
            "(*"+name+")()",
            pe.first,
            extendtype(thisscope, name+"_type")
        );
    }
    os<<"}"<<endl;
}



void SubsetParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& varname,
    const std::string& staticname,
    const std::string& thisscope
) const
{
    std::string myscope=extendtype(thisscope, cppTypeName(name));
    for (const ParameterSetEntry& pe: value)
    {
        std::string subname=pe.first;
        os<<"{\n";
        os<<"auto& "<<subname<<" = "<<varname<<".get< "<<pe.second->cppParamType(subname)<<" >(\""<<subname<<"\");\n";
        os<<"const auto& "<<subname<<"_static = "<<staticname<<"."<<subname<<";\n";
        pe.second->cppWriteSetStatement
        (
            os, subname, subname, subname+"_static", myscope
        );
        os<<"}\n"<<endl;
    }
}

void SubsetParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& varname,
    const std::string& staticname,
    const std::string& thisscope
) const
{
    std::string myscope=extendtype(thisscope, cppTypeName(name));
    for (const ParameterSetEntry& pe: value)
    {
        std::string subname=pe.first;
        os<<"{"<<endl;
        os<<"const auto& "<<subname<<" = "<<varname<<".get< "<<pe.second->cppParamType(subname)<<" >(\""<<subname<<"\");\n";
        os<<"auto& "<<subname<<"_static = "<<staticname<<"."<<subname<<";\n"<<endl;
        pe.second->cppWriteGetStatement
        (
            os, subname, subname, subname+"_static", myscope
        );
        os<<"}\n"<<endl;
    }
}
