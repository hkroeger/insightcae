#include "parametergenerator.h"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/lexical_cast.hpp"
#include <string>




arma::mat vec2mat(const std::vector<double>& vals)
{
    arma::mat m = arma::zeros(vals.size());
    for (size_t i=0; i<vals.size(); i++) m(i)=vals[i];
    return m;
}




PDLException::PDLException(const std::string& msg)
  : std::exception(),
    msg_(msg)
{}




const char* PDLException::what() const noexcept
{
  return msg_.c_str();
}




std::string extendtype(const std::string& pref, const std::string& app)
{
  if (pref=="")
  {
      return app;
  }
  else
  {
      auto p=pref;
      if (!boost::starts_with(p, "typename "))
          p="typename "+p;
      return p+"::"+app;
  }
}




void writeVec3Constant(std::ostream & os, const arma::mat &m)
{
    os << "arma::mat({";
    for (size_t i=0; i<m.n_elem; ++i)
    {
        if (i>0) os << ", ";
        os << m(i);
    }
    os << "}).t()";
}



defineType(ParameterGenerator);
defineStaticFunctionTableWithArgs(
    ParameterGenerator,
    insertrule, void,
    LIST(PDLParserRuleset& ruleset),
    LIST(ruleset)
    );


std::string joinPath(const std::string &c1, const std::string &c2)
{
    return c1 + ((!c1.empty()&&!c2.empty())?"/":"") + c2;
}





ParameterGenerator::ParameterGenerator(const std::string& d, bool h, bool e, bool n, int o)
: description(d),
  isHidden(h), isExpert(e), isNecessary(n), order(o)
{
    boost::replace_all(
        description,
        "\n", "\\n"
        );
}




ParameterGenerator::~ParameterGenerator()
{}




void ParameterGenerator::setName(const std::string &n)
{
    name=n;
}

void ParameterGenerator::setPath(const std::string &containerPath)
{
    parameterPath = joinPath(containerPath, name);
}

void ParameterGenerator::changeDescription(const std::string &newdesc)
{
    description=newdesc;
}




void ParameterGenerator::writeDoxygen(std::ostream& os) const
{
    // doxygen description
    os
        << "/**\n"
        << " * @brief\n"
        ;
    std::vector<std::string> lines;
    boost::split(lines, description, boost::is_any_of("\n"));
    for (const auto& line: lines)
        os << " * "<<line<<"\n";
    os   << " */\n";
}

/* c++
written by writeCppHeader:

 typdef xxx name_type;  // cppTypeDecl: return statement, cppType: xxx
 name_type name;

*/
void ParameterGenerator::cppAddRequiredInclude(std::set<std::string>&) const
{}




std::set<std::string> primitiveTypes{"int", "double", "float", "char", "bool", "void", "wchar"};

bool ParameterGenerator::isPrimitiveType() const
{
    return primitiveTypes.count(cppStaticType()) || boost::starts_with(cppStaticType(), "enum ");
}


std::string ParameterGenerator::cppInsightTypeConstructorParameters() const
{
    return "\"" + description + "\", "
           + (isHidden?"true":"false")+", "
           + (isExpert?"true":"false")+", "
           + (isNecessary?"true":"false")+", "
           + boost::lexical_cast<std::string>(order);
}

std::string ParameterGenerator::cppWrappedStaticType() const
{
    return
        std::string(isPrimitiveType()?"PrimitiveStaticValueWrap":"StaticValueWrap")
           +"< "+cppTypeName()+" >";
}




std::string ParameterGenerator::cppConstructorParameters() const
{
  return cppDefaultValueExpression();
}




std::string ParameterGenerator::cppTypeName() const
{
    return name+"_type";
}




void ParameterGenerator::writeCppTypeDecl(
    std::ostream& os ) const
{
    os << "typedef " <<cppStaticType()<< " " <<cppTypeName() <<";\n";
}




void ParameterGenerator::writeCppStaticVariableDefinition(
    std::ostream& os ) const
{

  // typedef
  writeCppTypeDecl(os);

  writeDoxygen(os);

  // variable declaration
  os << cppWrappedStaticType() << " " << name << ";\n";
}




/**
 * write the code to create a new parameter object for the dynamic parameter set
 */
void ParameterGenerator::cppWriteCreateStatement(
    std::ostream& os,
    const std::string& psvarname ) const
{
    os<<"auto "<<psvarname<<" = "
      "std::make_unique<"<<cppInsightType()<<">( "
       <<cppDefaultValueExpression()<<", "
       <<cppInsightTypeConstructorParameters()<<" ); ";
}




/**
 * write the code to insert a new parameter object into the dynamic parameter set
 */
void ParameterGenerator::cppWriteInsertStatement(
    std::ostream& os,
    const std::string& psvarname ) const
{
    os<<"{ ";
    this->cppWriteCreateStatement(os, name);
    os << psvarname << ".insert( \""<<name<<"\", std::move("<<name<<") ); ";
    os<<"}"<<std::endl;
}




/**
 * write the code to
 * transfer the values form the static c++ struct into the dynamic parameter set
 */
void ParameterGenerator::cppWriteSetStatement(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname ) const
{
    os<<varname<<".set("<<staticname<<");"<<std::endl;
}




/**
 * write the code to
 * transfer values from the dynamic parameter set into the static c++ data structure
 */
void ParameterGenerator::cppWriteGetStatement(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname ) const
{
    os
        <<staticname<< "="<<varname<<"();\n"
        <<staticname<< ".setPath( "<<varname<<" .path());\n";
}



std::ostream& operator<<(std::ostream& os, const ParameterGenerator& d)
{
    os<<"\""<<d.name<<"\"";
    return os;
}

