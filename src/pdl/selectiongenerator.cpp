#include "selectiongenerator.h"
#include "boost/algorithm/string/predicate.hpp"


using namespace std;


defineType(SelectionGenerator);
addToStaticFunctionTable(ParameterGenerator, SelectionGenerator, insertrule);


SelectionGenerator::SelectionGenerator(
    const std::vector<std::string>& sels,
    const std::string& sel, const std::string& d)
  : ParameterGenerator(d),
    selections(sels),
    selection(sel)
{}


void SelectionGenerator::cppAddRequiredInclude(std::set<std::string> &headers) const
{
  headers.insert("\"base/parameters/selectionparameter.h\"");
}


std::string SelectionGenerator::cppInsightType() const
{
    return "insight::SelectionParameter";
}


std::string SelectionGenerator::cppStaticType() const
{
  return "enum {"+boost::join(selections, ", ")+"}";
}


std::string SelectionGenerator::cppDefaultValueExpression() const
{
    auto v = extendtype(extendtype(thisscope, name+"_type"), selection);

    std::string pat("typename");
    if (boost::starts_with(v, pat))
        v.erase(0, pat.size());

    return v;
}



void SelectionGenerator::cppWriteCreateStatement(
    std::ostream& os,
    const std::string& psvarname ) const
{

  os<<"std::unique_ptr< "<<cppInsightType()<<" > "<<psvarname<<";"<<endl;
  os<<"{"<<endl;
  os<<"insight::SelectionParameter::ItemList items;"<<endl;
  for (const std::string& s: selections)
  {
    os<<"items.push_back(\""<<s<<"\");"<<endl;
  }
  os<< psvarname<<".reset(new "<<cppInsightType()<<"(\""<< selection <<"\", items, "
    << cppInsightTypeConstructorParameters()
    <<" )); "<<endl;
  os<<"}"<<endl;
}

void SelectionGenerator::cppWriteSetStatement(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
  os<<varname<<".set( int("<< staticname <<") );"<<endl;
}


void SelectionGenerator::cppWriteGetStatement(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname ) const
{
    os
        <<staticname<< "="<<cppTypeName()<<"("<<varname<<"());\n"
        <<staticname<< ".setPath( "<<varname<<" .path());\n";
}
