#ifndef INSIGHT_PARSER_GRAMMAR_H
#define INSIGHT_PARSER_GRAMMAR_H

#include <memory>

#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/thread/futures/future_error.hpp"
#include "boost/phoenix/core.hpp"
#include "boost/phoenix/function.hpp"

#include "boost/spirit/include/qi.hpp"
#include "boost/spirit/include/phoenix.hpp"
#include "boost/spirit/repository/include/qi_iter_pos.hpp"
#include "boost/phoenix/function/adapt_callable.hpp"

namespace insight {



namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx = boost::phoenix;




template<typename Iterator>
struct CurrentPos
{

    CurrentPos()
    {
        save_start_pos = qi::omit[repo::qi::iter_pos[
            phx::bind(&CurrentPos::setStartPos, this, qi::_1)]];
        current_pos = repo::qi::iter_pos[
            qi::_val = phx::bind(&CurrentPos::getCurrentPos, this, qi::_1)];
    }

    qi::rule<Iterator> save_start_pos;
    qi::rule<Iterator, std::size_t()> current_pos;

    void setStartPos(const Iterator &iterator) {
        start_pos_ = iterator;
    }

private:
    std::size_t getCurrentPos(const Iterator &iterator) {
        return std::distance(start_pos_, iterator);
    }

    Iterator start_pos_;
};




struct RuleContainerBase
{
    virtual ~RuleContainerBase();
};




template <class Rule>
struct RuleContainer
    : public RuleContainerBase,
      public std::shared_ptr<Rule>
{
    template<class RulePtr>
    RuleContainer(RulePtr r)
        : std::shared_ptr<Rule>(r)
    {}
};



/**
 * @brief The parser_grammar class
 * contains a boost::spirit grammar with a container
 * for additional rules with arbitrary signature
 */
template<class Grammar>
class ExtendedGrammar
: public Grammar
{
    boost::ptr_vector<RuleContainerBase> additionalRules_;
public:
    template<class ...Args>
    ExtendedGrammar(Args&&... args)
        : Grammar( std::forward<Args>(args)... )
    {}

    template<class Rule>
    inline Rule& addAdditionalRule(Rule *addrule)
    {
        additionalRules_.push_back(
            new RuleContainer<Rule>(
                addrule ) );
        return *addrule;
    }

    template<class Rule>
    inline Rule& addAdditionalRule(std::shared_ptr<Rule> addrule)
    {
        additionalRules_.push_back(
            new RuleContainer<Rule>(
                addrule ) );
        return *addrule;
    }
};




} // namespace insight

#endif // INSIGHT_PARSER_GRAMMAR_H
