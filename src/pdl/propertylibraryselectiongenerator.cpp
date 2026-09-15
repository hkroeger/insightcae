#include "propertylibraryselectiongenerator.h"




using namespace std;




defineType(PropertyLibrarySelectionGenerator);
addToStaticFunctionTable(ParameterGenerator, PropertyLibrarySelectionGenerator, insertrule);




PropertyLibrarySelectionGenerator
::PropertyLibrarySelectionGenerator(
    bool istempl,
    const std::string& libname,
    const std::string& deflSel,
    const std::string& d )
    : ParameterGenerator(d),
    isTemplate(istempl),
    libraryName(libname),
    defaultSelection(deflSel)
{}




void PropertyLibrarySelectionGenerator::cppAddRequiredInclude(
    std::set<std::string> &headers ) const
{
  headers.insert("\"base/parameters/propertylibraryselectionparameter.h\"");
  headers.insert("\"base/cppextensions.h\"");
}




std::string
PropertyLibrarySelectionGenerator::cppInsightType() const
{
    return "insight::PropertyLibrarySelectionParameter";
}




std::string PropertyLibrarySelectionGenerator::cppStaticType() const
{
  // type of the "parameters" field inside the generated struct (see writeCppTypeDecl)
  return "std::shared_ptr<insight::ParameterSet>";
}




std::string
PropertyLibrarySelectionGenerator::cppDefaultValueExpression() const
{
  std::string sel =
      (defaultSelection!="NODEFAULT")
        ? ("\""+defaultSelection+"\"")
        : (libraryName+"::library().entryList().front()");

  return
      "{ "+sel+", "
      "&"+libraryName+"::library().lookup("+sel+"), "
      +libraryName+"::library().defaultParameters("+sel+") }";
}




void PropertyLibrarySelectionGenerator::writeCppTypeDecl(
    std::ostream& os ) const
{
  std::string valueType = std::string(isTemplate?"typename ":"")+libraryName+"::value_type";
  std::string instanceType = std::string(isTemplate?"typename ":"")+libraryName+"::instance_type";

  os
      << "struct " << cppTypeName() << "\n"
      << "{\n"
      <<   "std::string selection;\n"
      <<   "const " << valueType << "* value;\n"
      <<   cppStaticType() << " parameters;\n"
      <<   "operator const " << valueType << "*() const { return value; }\n"
      <<   "std::shared_ptr<" << instanceType << "> createInstance() const\n"
      <<   "{\n"
      <<   "return " << libraryName << "::library().createInstance(selection, *parameters);\n"
      <<   "}\n"
      << "};\n";
}




void PropertyLibrarySelectionGenerator::cppWriteCreateStatement(
    std::ostream& os,
    const std::string& psvarname ) const
{
  os <<"std::unique_ptr< "<<cppInsightType()<<" > "<<psvarname<<";"<<endl;
  os <<"{"<<endl;
  os <<psvarname<<".reset(new "<<cppInsightType()
     <<"(";

  if (defaultSelection!="NODEFAULT")
  {
      os << "\""<<defaultSelection <<"\", ";
  }
  os
    << libraryName<<"::library(),\n"
    << cppInsightTypeConstructorParameters() <<"));\n"
    << "}\n";
}




void PropertyLibrarySelectionGenerator::cppWriteSetStatement(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname ) const
{
  os<<"{\n"
    <<varname<<".setSelection( "<<staticname<<".selection );\n"
    <<varname<<".instanceParameters().assignFrom( *"<<staticname<<".parameters );\n"
    <<"}\n";
}




void PropertyLibrarySelectionGenerator::cppWriteGetStatement(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname ) const
{
  os<<staticname<<".selection = "<<varname<<".selection();\n"
       <<staticname<<".value = &"<<libraryName<<"::library().lookup( "<<varname<<".selection() );\n"
       <<staticname<<".parameters = std::dynamic_unique_ptr_cast<insight::ParameterSet>( "<<varname<<".instanceParameters().clone() );\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n" ;
}
