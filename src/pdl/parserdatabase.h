#ifndef PARSERDATABASE_H
#define PARSERDATABASE_H

#include <algorithm>
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



arma::mat vec2mat(const std::vector<double>& vals);
BOOST_PHOENIX_ADAPT_FUNCTION(arma::mat, vec2mat_, vec2mat, 1);



class PDLException
{
  std::string msg_;
public:
  PDLException(const std::string& msg);

  const std::string& msg() const;
};




namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;




template <typename Iterator>
struct skip_grammar
        : public qi::grammar<Iterator>
{
    qi::rule<Iterator> skip;
    skip_grammar();
};




std::string extendtype(const std::string& pref, const std::string& app);



template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct PDLParser;




/**
 * Basic data structures
 */
class ParserDataBase
{

public:
    typedef std::shared_ptr<ParserDataBase> Ptr;

    std::string description;
    bool isHidden, isExpert, isNecessary;
    int order;

    inline void setHidden() { isHidden=true; }

    ParserDataBase(const std::string& d, bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0);

    virtual ~ParserDataBase();

    /* c++
    written by writeCppHeader:

     typdef xxx name_type;  // cppTypeDecl: return statement, cppType: xxx
     name_type name;

    */
    virtual void cppAddHeader(std::set<std::string>&) const;

    virtual std::string cppParamType(const std::string& name) const =0;
    virtual std::string cppType(const std::string& name) const =0;
    virtual std::string cppValueRep(const std::string& name) const =0;
    virtual std::string cppTypeName(const std::string& name) const;
    virtual std::string cppTypeDecl(const std::string& name) const;

    virtual void writeCppHeader(std::ostream& os, const std::string& name) const;

    /**
     * write the code to create a new parameter object for the dynamic parameter set
     */
    virtual void cppWriteCreateStatement
    (
        std::ostream& os,
        const std::string& name
    ) const;

    /**
     * write the code to insert a new parameter object into the dynamic parameter set
     */
    virtual void cppWriteInsertStatement
    (
        std::ostream& os,
        const std::string& psvarname,
        const std::string& name
    ) const;

    /**
     * write the code to
     * transfer the values form the static c++ struct into the dynamic parameter set
     */
    virtual void cppWriteSetStatement
    (
        std::ostream& os,
        const std::string&,
        const std::string& varname,
        const std::string& staticname,
        const std::string&
    ) const;

    /**
     * write the code to
     * transfer values from the dynamic parameter set into the static c++ data structure
     */
    virtual void cppWriteGetStatement
    (
        std::ostream& os,
        const std::string&,
        const std::string& varname,
        const std::string& staticname,
        const std::string&
    ) const;

};




typedef std::pair<std::string, ParserDataBase::Ptr> ParameterSetEntry;
typedef std::vector< ParameterSetEntry > ParameterSetData;




template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct PDLParserRuleset
{
    typedef qi::rule<Iterator, ParserDataBase::Ptr(), Skipper> ParameterDataRule;
    typedef std::shared_ptr<ParameterDataRule> ParameterDataRulePtr;

    qi::rule<Iterator, std::string(), Skipper> r_identifier, r_string, r_path, r_description_string, r_up_to_semicolon;
    qi::rule<Iterator, ParameterSetData(), Skipper> r_parameterset;
    qi::rule<Iterator, ParameterSetEntry(), Skipper> r_parametersetentry;

    qi::symbols<char, ParameterDataRulePtr> parameterDataRules;
    qi::rule<Iterator, ParserDataBase::Ptr(), Skipper, qi::locals<ParameterDataRulePtr> > r_parameterdata;

    PDLParserRuleset();

    void init() {}
};


#endif // PARSERDATABASE_H
