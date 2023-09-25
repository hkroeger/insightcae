#include "propertylibraryselectionparameterparser.h"


using namespace std;

defineType(PropertyLibrarySelectionParameterParser);
addToStaticFunctionTable(ParserDataBase, PropertyLibrarySelectionParameterParser, insertrule);


PropertyLibrarySelectionParameterParser::Data::Data(
    bool istempl,
    const std::string& libname,
    const std::string& deflSel,
    const std::string& d )
    : ParserDataBase(d),
    isTemplate(istempl),
    libraryName(libname),
    defaultSelection(deflSel)
{}

void PropertyLibrarySelectionParameterParser::Data::cppAddHeader(std::set<std::string> &headers) const
{
  headers.insert("\"base/parameters/propertylibraryselectionparameter.h\"");
}

std::string PropertyLibrarySelectionParameterParser::Data::cppType(const std::string&) const
{
  return std::string("const")+(isTemplate?" typename ":" ")+libraryName+"::value_type *";
}

std::string PropertyLibrarySelectionParameterParser::Data::cppValueRep(const std::string& name, const std::string& thisscope) const
{
  return "&"+libraryName+"::library().lookup("+
         (defaultSelection!="NODEFAULT"?
            "\""+defaultSelection+"\""
            :libraryName+"::library().entryList().front()"
          ) + ")";
}

std::string PropertyLibrarySelectionParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::PropertyLibrarySelectionParameter";
}

void PropertyLibrarySelectionParameterParser::Data::cppWriteCreateStatement(std::ostream& os, const std::string& name,
                                                             const std::string& thisscope) const
{
  os<<"std::unique_ptr< "<<cppParamType(name)<<" > "<<name<<";"<<endl;
  os<<"{"<<endl;
  os<<name<<".reset(new "<<cppParamType(name)<<"(";
  if (defaultSelection!="NODEFAULT")
  {
      os<< "\""<<defaultSelection <<"\", ";
  }
  os << libraryName<<"::library(), "
     << "\""<<description<<"\", "
     << (isHidden?"true":"false")<<","
     << (isExpert?"true":"false")<<","
     << (isNecessary?"true":"false")<<","
     <<order
     <<")); "<<endl;
  os<<"}"<<endl;
}

void PropertyLibrarySelectionParameterParser::Data::cppWriteSetStatement(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
  os<<varname<<".setSelection( "<<libraryName<<"::library().labelOf( *"<< staticname <<" ) );"<<endl;
}

void PropertyLibrarySelectionParameterParser::Data::cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
  const std::string& thisscope) const
{
  os<<staticname<<" = &"<<libraryName<<"::library().lookup( "<<varname<<".selection() );"<<endl;
}
