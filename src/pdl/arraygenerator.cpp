#include "arraygenerator.h"

using namespace std;


defineType(ArrayGenerator);
addToStaticFunctionTable(ParameterGenerator, ArrayGenerator, insertrule);



ArrayGenerator::ArrayGenerator(ParameterGeneratorPtr v, size_t n, const std::string& d)
: ParameterGenerator(d), value(v), num(n)
{}




void ArrayGenerator::setName(const std::string &n)
{
    value->setName(n+"_default");
    ParameterGenerator::setName(n);
}




void ArrayGenerator::cppAddRequiredInclude(std::set<std::string>& headers) const
{
  headers.insert("<vector>");
  headers.insert("\"base/parameters/arrayparameter.h\"");
  value->cppAddRequiredInclude(headers);
}




std::string ArrayGenerator::cppInsightType() const
{
    return "insight::ArrayParameter";
}




std::string ArrayGenerator::cppStaticType() const
{
    return "std::vector< "+value->cppWrappedStaticType()+" >";
}




std::string ArrayGenerator::cppDefaultValueExpression() const
{
    std::vector<std::string> items;
    for (size_t i=0; i<num; i++)
    {
#warning add path
        items.push_back(value->cppConstructorParameters()) ;
    }

    return cppStaticType() + "{" +boost::join(items, ", ")+" }";
}




void ArrayGenerator::writeCppTypeDecl(
    std::ostream& os) const
{
    value->writeCppTypeDecl (os );

    ParameterGenerator::writeCppTypeDecl ( os );
}




void ArrayGenerator::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& psvarname
) const
{

  os<<"auto "<<psvarname<<" = "
        "std::make_unique< "<<cppInsightType()<<" >"
        "("<<cppInsightTypeConstructorParameters()<<");\n"

    <<"{\n";

  value->cppWriteCreateStatement(os, value->name+"_default");
  os<<psvarname<<"->setDefaultValue(*"<<value->name<<"_default);\n";

  if (num>0)
  {
    os << "for (size_t i=0; i<"<<num<<"; i++)\n"
       << "{\n"
       <<  psvarname<<"->appendEmpty();\n"
       << "}\n";
  }
  os<<"}\n";
}




void ArrayGenerator::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
  os << varname<<".clear();\n";

  os << "for (size_t k=0; k<"<<staticname<<".size(); k++)\n";
  os << "{\n";

  os <<  varname<<".appendEmpty();"<<endl;

  os <<  "auto& "<<varname<<"_cur = "
     <<     "dynamic_cast< "<< value->cppInsightType() <<"& >"
     <<      "("<<varname<<"[k]);\n";

  os <<  "const auto& "<<varname<<"_cur_static = "
     <<      staticname<<"[k];\n";

  value->cppWriteSetStatement(os, varname+"_cur", varname+"_cur_static");

  os<<"}\n";
}




void ArrayGenerator::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
  os << staticname<<".resize( "<<varname<<".size() );\n"
     << staticname<<".setPath(" <<varname<<".path() );\n"
     << "for(int k=0; k<"<<varname<<".size(); k++) \n"
     << "{\n"
     <<  "const auto& "<<varname<<"_cur = "
     <<   "dynamic_cast<const "<< value->cppInsightType() <<"& >("<<varname<<"[k]);\n"
     <<  "auto& "<<varname<<"_cur_static = "<<staticname<<"[k];\n";

     value->cppWriteGetStatement(os, varname+"_cur", varname+"_cur_static");

  os << "}\n";
}

