#include "includedsubsetparameterparser.h"

using namespace std;


IncludedSubsetParameterParser::Data::Data(const std::string& v, const std::string& d, const DefaultValueModifications& defmod)
    : ParserDataBase(d), value(v), default_value_modifications(defmod)
{}

std::string IncludedSubsetParameterParser::Data::cppType(const std::string&) const
{
    return value;
}

std::string IncludedSubsetParameterParser::Data::cppValueRep(const std::string&) const
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
    const std::string& name
) const
{
    os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name
    <<"(new "<<cppParamType(name)<<"("<< value <<"::makeDefault(), \""<<description<<"\")); "<<endl;

    for (const DefaultModification& dm: default_value_modifications)
    {
      std::string type = boost::fusion::at_c<0>(dm);
      std::string key = boost::fusion::at_c<1>(dm);
      std::string value = boost::fusion::at_c<2>(dm);
      if (type=="double")
      {
        os << name << "->get<DoubleParameter>(\""<<key<<"\")() = "<<value<<";\n";
      }
      else if (type=="int")
      {
        os << name << "->get<IntParameter>(\""<<key<<"\")() = "<<value<<";\n";
      }
      else if (type=="bool")
      {
        os << name << "->get<BoolParameter>(\""<<key<<"\")() = "<<value<<";\n";
      }
      else if (type=="string")
      {
        os << name << "->get<StringParameter>(\""<<key<<"\")() = \""<<value<<"\";\n";
      }
      else if (type=="path")
      {
        os << name << "->get<PathParameter>(\""<<key<<"\")() = \""<<value<<"\";\n";
      }
      else if (type=="selection")
      {
        os << name << "->get<SelectionParameter>(\""<<key<<"\").setSelection(\""<<value<<"\");\n";
      }
      else if (type=="vector")
      {
        std::istringstream is(value);
        double x, y, z;
        is >> x >> y >> z;
        if (is.fail()) throw PDLException("Could not interpret vector values in string: \""+value+"\"");
        os << name << "->get<VectorParameter>(\""<<key<<"\")()=vec3("<<x<<","<<y<<","<<z<<");\n";
      }
      else if (type=="selectablesubset")
      {
        os << name << "->get<SelectableSubsetParameter>(\""<<key<<"\").selection() =   \""<<value<<"\";\n";
      }
      else
        throw PDLException("Modification of parameter of type "+type+" during subset inclusion is currently not supported!");
    }
}

void IncludedSubsetParameterParser::Data::cppWriteInsertStatement
(
    std::ostream& os,
    const std::string& psvarname,
    const std::string& name
) const
{
    os<<"{ ";
    os<<" std::string key(\""<<name<<"\"); ";
    this->cppWriteCreateStatement(os, name);

    os<<" if ("<<psvarname<<".find(key)!="<<psvarname<<".end()) {\n";
    os<<  psvarname<<".getSubset(key).merge(*"<<name<<"); ";
    os<<" } else {"<<endl;
    os<<  psvarname<<".insert(key, "<<name<<");\n";
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
