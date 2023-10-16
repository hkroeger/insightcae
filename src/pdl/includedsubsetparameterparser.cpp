#include "includedsubsetparameterparser.h"

using namespace std;

defineType(IncludedSubsetParameterParser);
addToStaticFunctionTable(ParserDataBase, IncludedSubsetParameterParser, insertrule);

IncludedSubsetParameterParser::Data::Data(const std::string& v, const std::string& d, const DefaultValueModifications& defmod)
    : ParserDataBase(d), value(v), default_value_modifications(defmod)
{}

void IncludedSubsetParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
  headers.insert("\"base/parameters/subsetparameter.h\"");
}

std::string IncludedSubsetParameterParser::Data::cppType(const std::string&) const
{
    return value;
}

std::string IncludedSubsetParameterParser::Data::cppValueRep(const std::string&, const std::string& thisscope) const
{
    return value+"()";
}

std::string IncludedSubsetParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::SubsetParameter";
}

void IncludedSubsetParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& thisscope
) const
{
    os<<"std::unique_ptr< "<<cppParamType(name)<<" > "<<name
    <<"(new "<<cppParamType(name)<<"("<< value <<"::makeDefault(), \""<<description<<"\")); "<<endl;

    for (const DefaultModification& dm: default_value_modifications)
    {
      std::string type = boost::fusion::at_c<0>(dm);
      std::string key = boost::fusion::at_c<1>(dm);
      std::string value = boost::fusion::at_c<2>(dm);
      if (type=="double")
      {
        os << name << "->get<DoubleParameter>(\""<<key<<"\").set("<<value<<");\n";
      }
      else if (type=="int")
      {
        os << name << "->get<IntParameter>(\""<<key<<"\").set("<<value<<");\n";
      }
      else if (type=="bool")
      {
        os << name << "->get<BoolParameter>(\""<<key<<"\").set("<<value<<");\n";
      }
      else if (type=="string")
      {
        os << name << "->get<StringParameter>(\""<<key<<"\").set(\""<<value<<"\");\n";
      }
      else if (type=="path")
      {
        os << name << "->get<PathParameter>(\""<<key<<"\").set(\""<<value<<"\");\n";
      }
      else if (type=="selection")
      {
        os << name << "->get<SelectionParameter>(\""<<key<<"\").setSelection(\""<<value<<"\");\n";
      }
      else if (type=="vector")
      {
        std::vector<std::string> cmpts;
        boost::algorithm::trim(value);
        boost::split(
              cmpts,
              value,
              boost::is_any_of(" \t"),
              boost::token_compress_on
         );
        os << name << "->get<VectorParameter>(\""<<key<<"\").set({"+boost::join(cmpts, ",")+"});\n";
      }
      else if (type=="selectablesubset")
      {
        os << name << "->get<SelectableSubsetParameter>(\""<<key<<"\").setSelection(\""<<value<<"\");\n";
      }
      else
        throw PDLException("Modification of parameter of type "+type+" during subset inclusion is currently not supported!");
    }
}

void IncludedSubsetParameterParser::Data::cppWriteInsertStatement
(
    std::ostream& os,
    const std::string& psvarname,
    const std::string& name,
    const std::string& thisscope
) const
{
    os<<"{ ";
    os<<" std::string key(\""<<name<<"\"); ";
    this->cppWriteCreateStatement(os, name, extendtype(thisscope, name+"_type"));

    os<<" if ("<<psvarname<<".find(key)!="<<psvarname<<".end()) {\n";
    os<<  psvarname<<".getSubset(key).merge(*"<<name<<"); ";
    os<<" } else {"<<endl;
    os<<  psvarname<<".emplace(key, std::move("<<name<<"));\n";
    os<<" }"<<endl;
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
