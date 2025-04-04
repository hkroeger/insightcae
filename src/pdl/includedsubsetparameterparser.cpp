#include "includedsubsetparameterparser.h"

using namespace std;

defineType(IncludedSubsetGenerator);
addToStaticFunctionTable(ParameterGenerator, IncludedSubsetGenerator, insertrule);

IncludedSubsetGenerator::IncludedSubsetGenerator(
    const std::string& v,
    const std::string& d,
    const DefaultValueModifications& defmod)
  : ParameterGenerator(d),
    value(v),
    default_value_modifications(defmod)
{}


void IncludedSubsetGenerator::cppAddRequiredInclude(
    std::set<std::string>& headers) const
{
  headers.insert("\"base/parameters/subsetparameter.h\"");
}

std::string IncludedSubsetGenerator::cppInsightType() const
{
    return "insight::ParameterSet";
}

std::string IncludedSubsetGenerator::cppStaticType() const
{
    return "typename "+value;
}

std::string IncludedSubsetGenerator::cppDefaultValueExpression() const
{
    return cppStaticType()+"()";
}

void IncludedSubsetGenerator::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& psvarname
) const
{
    os<<"auto "<<psvarname<<" = "
    <<cppInsightType()<<"::create_uninitialized("
       << value <<"::makeDefault()->entries(), "
       <<cppInsightTypeConstructorParameters()<<");\n";

    for (const DefaultModification& dm: default_value_modifications)
    {
      std::string type = boost::fusion::at_c<0>(dm);
      std::string key = boost::fusion::at_c<1>(dm);
      std::string value = boost::fusion::at_c<2>(dm);
      if (type=="double")
      {
        os << psvarname << "->get<DoubleParameter>(\""<<key<<"\").set("<<value<<");\n";
      }
      else if (type=="int")
      {
        os << psvarname << "->get<IntParameter>(\""<<key<<"\").set("<<value<<");\n";
      }
      else if (type=="bool")
      {
        os << psvarname << "->get<BoolParameter>(\""<<key<<"\").set("<<value<<");\n";
      }
      else if (type=="string")
      {
        os << psvarname << "->get<StringParameter>(\""<<key<<"\").set(\""<<value<<"\");\n";
      }
      else if (type=="path")
      {
        os << psvarname << "->get<PathParameter>(\""<<key<<"\").set(\""<<value<<"\");\n";
      }
      else if (type=="selection")
      {
        os << psvarname << "->get<SelectionParameter>(\""<<key<<"\").setSelection(\""<<value<<"\");\n";
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
        os << psvarname << "->get<VectorParameter>(\""<<key<<"\").set({"+boost::join(cmpts, ",")+"});\n";
      }
      else if (type=="selectablesubset")
      {
        os << psvarname << "->get<SelectableSubsetParameter>(\""<<key<<"\").setSelection(\""<<value<<"\");\n";
      }
      else
        throw PDLException("Modification of parameter of type "+type+" during subset inclusion is currently not supported!");
    }
}

void IncludedSubsetGenerator::cppWriteInsertStatement
(
    std::ostream& os,
    const std::string& psvarname
) const
{
    os<<"{ ";
    os<<" std::string key(\""<<name<<"\"); ";
    this->cppWriteCreateStatement(os, name/*, extendtype(thisscope, name+"_type")*/);

    os<<" if ("<<psvarname<<".contains(key)) {\n";
    os<<  psvarname<<".getSubset(key).merge(*"<<name<<"); ";
    os<<" } else {"<<endl;
    os<<  psvarname<<".insert(key, std::move("<<name<<"));\n";
    os<<" }"<<endl;
    os<<"}"<<endl;
}


void IncludedSubsetGenerator::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os << staticname << ".set(" << varname << ");" << endl;
}

void IncludedSubsetGenerator::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os <<staticname<< ".get(" << varname << ");\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n";
}
