#include "parserdatabase.h"

arma::mat vec2mat(const std::vector<double>& vals)
{
    arma::mat m = arma::zeros(vals.size());
    for (size_t i=0; i<vals.size(); i++) m(i)=vals[i];
    return m;
}

PDLException::PDLException(const std::string& msg)
  : msg_(msg)
{
}

const std::string& PDLException::msg() const
{
  return msg_;
}

std::string extendtype(const std::string& pref, const std::string& app)
{
  if (pref=="") return app;
  else return pref+"::"+app;
}



ParserDataBase::ParserDataBase(const std::string& d, bool h, bool e, bool n, int o)
: description(d),
  isHidden(h), isExpert(e), isNecessary(n), order(o)
{
    boost::replace_all(description,
                 "\n", "\\n");
}

ParserDataBase::~ParserDataBase() {}

/* c++
written by writeCppHeader:

 typdef xxx name_type;  // cppTypeDecl: return statement, cppType: xxx
 name_type name;

*/
void ParserDataBase::cppAddHeader(std::set<std::string>&) const
{}


std::string ParserDataBase::cppTypeName(const std::string& name) const
{
    return name+"_type";
}

std::string ParserDataBase::cppTypeDecl(const std::string& name) const
{
    return std::string("typedef ")+cppType(name)+" "+cppTypeName(name)+";";
}

void ParserDataBase::writeCppHeader(std::ostream& os, const std::string& name) const
{
    os<<cppTypeDecl(name)<<std::endl;
    os<<cppTypeName(name)+" "<<name<<";"<<std::endl;
}

/**
 * write the code to create a new parameter object for the dynamic parameter set
 */
void ParserDataBase::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name
) const
{
    os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<"("
      "new "<<cppParamType(name)<<"("<<cppValueRep(name)<<", \""<<description<<"\", "
      << (isHidden?"true":"false")<<","
      << (isExpert?"true":"false")<<","
      << (isNecessary?"true":"false")<<","
      <<order
      <<")"
      "); ";
}

/**
 * write the code to insert a new parameter object into the dynamic parameter set
 */
void ParserDataBase::cppWriteInsertStatement
(
    std::ostream& os,
    const std::string& psvarname,
    const std::string& name
) const
{
    os<<"{ ";
    os<<"std::string key(\""<<name<<"\"); ";
    this->cppWriteCreateStatement(os, name);
    os<<psvarname<<".insert(key, "<<name<<"); ";
    os<<"}"<<std::endl;
}

/**
 * write the code to
 * transfer the values form the static c++ struct into the dynamic parameter set
 */
void ParserDataBase::cppWriteSetStatement
(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<varname<<"() = "<<staticname<<";"<<std::endl;
}

/**
 * write the code to
 * transfer values from the dynamic parameter set into the static c++ data structure
 */
void ParserDataBase::cppWriteGetStatement
(
    std::ostream& os,
    const std::string&,
    const std::string& varname,
    const std::string& staticname,
    const std::string&
) const
{
    os<<staticname<<" = "<<varname<<"();"<<std::endl;
}
