#include "doublerangeparameterparser.h"

using namespace std;

defineType(DoubleRangeParameterParser);
addToStaticFunctionTable(ParserDataBase, DoubleRangeParameterParser, insertrule);

DoubleRangeParameterParser::Data::Data(const std::vector<double>& v, const std::string& d)
: ParserDataBase(d), value(v)
{}

void DoubleRangeParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
  headers.insert("<set>");
  headers.insert("\"base/parameters/doublerangeparameter.h\"");
}

std::string DoubleRangeParameterParser::Data::cppType(const std::string&) const
{
  return "std::set<double>";
}

std::string DoubleRangeParameterParser::Data::cppParamType(const std::string&) const
{
  return "insight::DoubleRangeParameter";
}

std::string DoubleRangeParameterParser::Data::cppValueRep(const std::string&, const std::string& thisscope) const
{
      std::ostringstream os;
//      os<<"boost::assign::list_of";
//      for (size_t i=0; i<value.size(); i++)
//      {
//        os<<"("<<value[i]<<")";
//      }
//      os<<".convert_to_container<std::set<double> >()";
      os<<"{";
      for (size_t i=0; i<value.size(); i++)
      {
          if (i>0) os<<", ";
          os<<value[i];
      }
      os<<"}";
      return os.str();
}

void DoubleRangeParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<varname<<".resetValues("<<staticname<<");"<<endl;
}

void DoubleRangeParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<staticname<<" = "<<varname<<".values();"<<endl;
}
