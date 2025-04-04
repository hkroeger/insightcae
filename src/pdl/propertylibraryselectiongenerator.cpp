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




bool PropertyLibrarySelectionGenerator::isPrimitiveType() const
{
    return true;
}




void PropertyLibrarySelectionGenerator::cppAddRequiredInclude(
    std::set<std::string> &headers ) const
{
  headers.insert("\"base/parameters/propertylibraryselectionparameter.h\"");
}




std::string
PropertyLibrarySelectionGenerator::cppInsightType() const
{
    return "insight::PropertyLibrarySelectionParameter";
}




std::string PropertyLibrarySelectionGenerator::cppStaticType() const
{
  return std::string("const")+(isTemplate?" typename ":" ")+libraryName+"::value_type *";
}




std::string
PropertyLibrarySelectionGenerator::cppDefaultValueExpression() const
{
  return "&"+libraryName+"::library().lookup("+
         (defaultSelection!="NODEFAULT"?
            "\""+defaultSelection+"\""
            :libraryName+"::library().entryList().front()");
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
  os<<varname<<".setSelection( "<<libraryName<<"::library().labelOf( *"<< staticname <<" ) );"<<endl;
}




void PropertyLibrarySelectionGenerator::cppWriteGetStatement(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname ) const
{
  os<<staticname<<" = &"<<libraryName<<"::library().lookup( "<<varname<<".selection() );\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n" ;
}
