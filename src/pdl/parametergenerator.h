#ifndef PARAMETERGENERATOR_H
#define PARAMETERGENERATOR_H

#include <algorithm>
#include <memory>
#include <string>

#define BOOST_NO_CXX11_SCOPED_ENUMS

#include "boost/filesystem.hpp"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "boost/assign.hpp"
#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/format.hpp"
#include <boost/tokenizer.hpp>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/algorithm/string.hpp>
#include "boost/date_time.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/regex.hpp>
#include "boost/concept_check.hpp"
#include "boost/utility.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/ptr_container/ptr_map.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/foreach.hpp"
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/buffer_concepts.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include "boost/variant.hpp"
#include "boost/variant/recursive_variant.hpp"
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/adapted.hpp>
#include "boost/thread.hpp"

#include <armadillo>

#define BOOST_SPIRIT_USE_PHOENIX_V3
// #define BOOST_SPIRIT_DEBUG

#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>


#include "../toolkit/base/factory.h"



arma::mat vec2mat(const std::vector<double>& vals);
BOOST_PHOENIX_ADAPT_FUNCTION(arma::mat, vec2mat_, vec2mat, 1);


class ParameterGenerator;



class PDLException
    : public std::exception
{
  std::string msg_;
public:
  PDLException(const std::string& msg);

  const char* what() const noexcept override;
};


void writeVec3Constant(std::ostream&, const arma::mat& m);


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;




//template <typename Iterator>
struct skip_grammar
 : public qi::grammar<std::string::iterator>
{
    qi::rule<std::string::iterator> skip;
    skip_grammar();
};




std::string extendtype(const std::string& pref, const std::string& app);



struct PDLParser;
struct PDLParserRuleset;

std::string joinPath(const std::string& c1, const std::string& c2);

/**
 * Basic data structures
 */
class ParameterGenerator
{

public:
    declareType("ParameterGenerator");

    declareStaticFunctionTableWithArgs(
        insertrule,
        void,
        LIST(PDLParserRuleset&),
        LIST(PDLParserRuleset& ruleset)
        );

    std::string name;
    std::string description;
    bool isHidden, isExpert, isNecessary;
    int order;

#warning TBD
    std::string thisscope, parameterPath;

    inline void setHidden() { isHidden=true; }
    inline void setExpert() { isExpert=true; }
    inline void setNecessary() { isNecessary=true; }


    ParameterGenerator(
        const std::string& d,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    virtual ~ParameterGenerator();

    virtual void setName(const std::string& name);
    virtual void setPath(const std::string& containerPath);
    void changeDescription(const std::string& newdesc);


    virtual void writeDoxygen(std::ostream& os) const;

    /* c++
    written by writeCppHeader:

     typdef xxx name_type;  // cppTypeDecl: return statement, cppType: xxx
     name_type name;

    */    
    virtual void cppAddRequiredInclude(std::set<std::string>&) const;

    virtual bool isPrimitiveType() const;

    virtual std::string cppInsightType() const =0;
    virtual std::string cppInsightTypeConstructorParameters() const;
    virtual std::string cppStaticType() const =0;
    virtual std::string cppDefaultValueExpression() const =0;

    virtual std::string cppWrappedStaticType() const;

    virtual std::string cppConstructorParameters() const;
    virtual std::string cppTypeName() const;

    virtual void writeCppTypeDecl (
        std::ostream& os ) const;

    void writeCppStaticVariableDefinition (
        std::ostream& os ) const;

    /**
     * write the code to create a new parameter object for the dynamic parameter set
     */
    virtual void cppWriteCreateStatement (
        std::ostream& os,
        const std::string& psvarname ) const;

    /**
     * write the code to insert a new parameter object into the dynamic parameter set
     */
    virtual void cppWriteInsertStatement (
        std::ostream& os,
        const std::string& psvarname ) const;

    /**
     * write the code to
     * transfer the values form the static c++ struct into the dynamic parameter set
     */
    virtual void cppWriteSetStatement (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname ) const;

    /**
     * write the code to
     * transfer values from the dynamic parameter set into the static c++ data structure
     */
    virtual void cppWriteGetStatement (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname ) const;

};


typedef std::shared_ptr<ParameterGenerator> ParameterGeneratorPtr;
typedef ParameterGeneratorPtr ParameterSetEntry;
typedef std::vector< ParameterSetEntry > ParameterSetData;



class SubsetGenerator;


typedef boost::fusion::vector2<
    bool,
    std::shared_ptr<SubsetGenerator>,
    bool
    > PDLParserResult;


struct PDLParserRuleset
{
    typedef std::string::iterator Iterator;
    typedef skip_grammar Skipper;

    typedef qi::rule<Iterator, ParameterGeneratorPtr(), Skipper> ParameterDataRule;
    typedef std::shared_ptr<ParameterDataRule> ParameterDataRulePtr;

    qi::rule<Iterator, std::string(), Skipper>
        r_identifier, r_string, r_path, r_description_string, r_up_to_semicolon, r_templateArg;
    qi::rule<Iterator, ParameterSetEntry(), Skipper> r_parametersetentry;

    qi::symbols<char, ParameterDataRulePtr> parameterDataRules;
    qi::rule<Iterator, ParameterGeneratorPtr(), Skipper, qi::locals<ParameterDataRulePtr> > r_parameterdata;
    qi::rule<Iterator, std::string(), Skipper> r_addcode;

    qi::rule<Iterator, std::shared_ptr<SubsetGenerator>(), Skipper> r_parameterset;

    qi::rule<Iterator, PDLParserResult(), Skipper> r_pdl_content;

    PDLParserRuleset();

    void init() {}
};


std::ostream& operator<<(std::ostream& os, const ParameterGenerator& d);

#endif // PARAMETERGENERATOR_H
