/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

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

#include <armadillo>

/**
 * \defgroup PDL PDL - Parameter Definition Language
 * 
 * PDL is a special syntax to describe input parameter sets.
 * The language is parsed by a PDL compiler who translates it into special classes which..
 */

arma::mat vec2mat(const std::vector<double>& vals)
{
 arma::mat m = arma::zeros(vals.size());
 for (size_t i=0; i<vals.size(); i++) m(i)=vals[i];
 return m;
}

BOOST_PHOENIX_ADAPT_FUNCTION(arma::mat, vec2mat_, vec2mat, 1);

using namespace std;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;
using namespace boost;


template <typename Iterator>
struct skip_grammar 
: public qi::grammar<Iterator>
{
        qi::rule<Iterator> skip;
        skip_grammar();
};


std::string extendtype(const std::string& pref, const std::string& app)
{
  if (pref=="") return app;
  else return pref+"::"+app;
}

/** 
 * Basic data structures 
 */
class ParserDataBase
{
public:
  typedef boost::shared_ptr<ParserDataBase> Ptr;
  
  std::string description;

  ParserDataBase(const std::string& d) : description(d) {}
  
  /* c++
  written by writeCppHeader:
  
   typdef xxx name_type;  // cppTypeDecl: return statement, cppType: xxx
   name_type name;
   
  */ 
  virtual void cppAddHeader(std::set<std::string>& headers) const {};
  virtual std::string cppParamType(const std::string& name) const =0;
  virtual std::string cppType(const std::string& name) const =0;
  virtual std::string cppValueRep(const std::string& name) const =0;
  virtual std::string cppTypeName(const std::string& name) const { return name+"_type"; }
  virtual std::string cppTypeDecl(const std::string& name) const 
  {
    return std::string("typedef ")+cppType(name)+" "+cppTypeName(name)+";";
  }
  
  virtual void writeCppHeader(std::ostream& os, const std::string& name) const
  {
    os<<cppTypeDecl(name)<<endl;
    os<<cppTypeName(name)+" "<<name<<";"<<endl;
  }
  
  /*
  written by writeCppSetStatement:
  
    .. ( ParameterSet &p)
   ...
   {
     // SetStatement
     p.get<TYPE>(PREFIX+)() = VALUE;
   }
  */
  
    virtual void cppWriteCreateStatement(std::ostream& os, const std::string& name) const
    {
      os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<"("
	  "new "<<cppParamType(name)<<"("<<cppValueRep(name)<<", \""<<description<<"\")"
	  "); ";
    }
    
    virtual void cppWriteInsertStatement(std::ostream& os, const std::string& psvarname, const std::string& name) const
    {
      os<<"{ ";
      os<<"std::string key(\""<<name<<"\"); ";
      this->cppWriteCreateStatement(os, name);
      os<<psvarname<<".insert(key, "<<name<<"); ";
      os<<"}"<<endl;
    }

    virtual void cppWriteSetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
      const std::string& typepref
    ) const
    {
      os<<varname<<"() = "<<staticname<<";"<<endl;
    }
    
    virtual void cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
      const std::string& typepref
    ) const
    {
      os<<staticname<<" = "<<varname<<"();"<<endl;
    }
    
};

typedef std::pair<std::string, ParserDataBase::Ptr> ParameterSetEntry;
typedef std::vector< ParameterSetEntry > ParameterSetData;

template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct PDLParser;

template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
struct PDLParserRuleset
{
  typedef qi::rule<Iterator, ParserDataBase::Ptr(), Skipper> ParameterDataRule;
  typedef boost::shared_ptr<ParameterDataRule> ParameterDataRulePtr;

  qi::rule<Iterator, std::string(), Skipper> r_identifier, r_string, r_description_string;
  qi::rule<Iterator, ParameterSetData(), Skipper> r_parameterset;
  qi::rule<Iterator, ParameterSetEntry(), Skipper> r_parametersetentry;
  
  qi::symbols<char, ParameterDataRulePtr> parameterDataRules;
  qi::rule<Iterator, ParserDataBase::Ptr(), Skipper, qi::locals<ParameterDataRulePtr> > r_parameterdata;
  
  PDLParserRuleset();
  
  void init() {}
  
  void addIncludeRule();
  
};



template <typename Iterator>
skip_grammar<Iterator>::skip_grammar() 
 : skip_grammar::base_type(skip, "PL/0")
{
    skip
	=   boost::spirit::ascii::space
	| repo::confix("//", qi::eol)[*(qi::char_ - qi::eol)]
	| repo::confix("#", qi::eol)[*(qi::char_ - qi::eol)]
	;
}
/**
 * \addtogroup PDL
 * \section grammar Grammar
 * \subsection grammar_comments Comments
 * Comments are supported inside the definition block:
 * - lines starting with //
 * - lines starting with #
 */


template <typename Iterator, typename Skipper>
PDLParserRuleset<Iterator,Skipper>::PDLParserRuleset()
{  
  r_string = as_string[ lexeme [ "\"" >> *~char_("\"") >> "\"" ] ];
  r_description_string = (r_string | attr(""));
  r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
  
  r_parameterdata %= omit[ parameterDataRules[ qi::_a = qi::_1 ] ] >> qi::lazy(*qi::_a);
  
//   parameterDataRules.add
//   (
//     "include",
//     typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
//       ( "(" >> r_string >> ")" >> ruleset.r_description_string ) 
//       [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(phx::construct<arma::mat>(qi::_1), qi::_2)) ]
//     ))
//   );
    
  r_parametersetentry = r_identifier >> '=' >> r_parameterdata;
  r_parameterset = *( r_parametersetentry );
  
//   BOOST_SPIRIT_DEBUG_NODE(r_identifier);
//   BOOST_SPIRIT_DEBUG_NODE(r_parameterdata);
//   BOOST_SPIRIT_DEBUG_NODE(r_parameterset);
//   BOOST_SPIRIT_DEBUG_NODE(r_parametersetentry);
}
/**
 * \addtogroup PDL
 * \subsection grammar_primitives Primitives
 * - Strings are any char confined inside quotes ""
 * - Descriptor strings may be omitted. In this case, an empty descriptor is inserted.
 * - Identifiers must not be enclosed by quotes but have to start with an alphabetical character. 
 *   They may then contain alphanumerical chars or underscores.
 */


/* specialized data structures */
struct BoolParameterParser
{
  struct Data
  : public ParserDataBase
  {
    bool value;
    
    Data(bool v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
    virtual std::string cppType(const std::string&) const
    {
      return "bool";
    }
    virtual std::string cppParamType(const std::string& name) const { return "insight::BoolParameter"; };   
    virtual std::string cppValueRep(const std::string& name) const { return boost::lexical_cast<std::string>(value); }
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "bool",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( qi::bool_ >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};

struct DoubleParameterParser
{
  struct Data
  : public ParserDataBase
  {
    double value;
    
    Data(double v, const std::string& d)
    : ParserDataBase(d), value(v)
    {}
    
    virtual std::string cppType(const std::string&) const
    {
      return "double";
    }
    virtual std::string cppParamType(const std::string& name) const { return "insight::DoubleParameter"; };   
    virtual std::string cppValueRep(const std::string& name) const { return boost::lexical_cast<std::string>(value); }
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "double",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( qi::double_ >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};



struct VectorParameterParser
{
  struct Data
  : public ParserDataBase
  {
    arma::mat value;
    
    Data(const arma::mat& v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
    virtual void cppAddHeader(std::set<std::string>& headers) const 
    {
      headers.insert("<armadillo>");
    };
    
    virtual std::string cppType(const std::string&) const
    {
      return "arma::mat";
    }

    virtual std::string cppParamType(const std::string& name) const { return "insight::VectorParameter"; }; 
    
    virtual std::string cppValueRep(const std::string& name) const 
    { 
      std::ostringstream os;
      os<<"arma::mat(boost::assign::list_of";
      for (int i=0; i<value.n_elem; i++)
      {
	os<<"("<<value(i)<<")";
      }
      os<<".convert_to_container<std::vector<double> >().data(), "<<value.n_elem<<", 1)";
      return os.str();
    }
    
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "vector",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( "(" >> *double_ >> ")" >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(
           new_<Data>(vec2mat_(qi::_1), qi::_2)
          ) ]
      ))
    );
  }
};

struct IntParameterParser
{
  struct Data
  : public ParserDataBase
  {
    int value;
    
    Data(int v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
    virtual std::string cppType(const std::string&) const
    {
      return "int";
    }

    virtual std::string cppParamType(const std::string& name) const { return "insight::IntParameter"; };   
    virtual std::string cppValueRep(const std::string& name) const { return boost::lexical_cast<std::string>(value); }
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "int",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( qi::int_ >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};

struct StringParameterParser
{
  struct Data
  : public ParserDataBase
  {
    std::string value;
    
    Data(const std::string& v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
    virtual void cppAddHeader(std::set< std::string >& headers) const
    {
      headers.insert("<string>");
    }
    
    virtual std::string cppType(const std::string&) const
    {
      return "std::string";
    }

    virtual std::string cppParamType(const std::string& name) const { return "insight::StringParameter"; };   
    virtual std::string cppValueRep(const std::string& name) const { return "\""+boost::lexical_cast<std::string>(value)+"\""; }

  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "string",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( ruleset.r_string >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};


struct PathParameterParser
{
  struct Data
  : public ParserDataBase
  {
    boost::filesystem::path value;
    
    Data(const boost::filesystem::path& v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
    virtual void cppAddHeader(std::set< std::string >& headers) const
    {
      headers.insert("<boost/filesystem.hpp>");
    }
    
    virtual std::string cppType(const std::string&) const
    {
      return "boost::filesystem::path";
    }

    virtual std::string cppParamType(const std::string& name) const { return "insight::PathParameter"; };   
    virtual std::string cppValueRep(const std::string& name) const { return boost::lexical_cast<std::string>(value); }

  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "path",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( ruleset.r_string >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
      ))
    );
  }
};

struct SelectionParameterParser
{
  struct Data
  : public ParserDataBase
  {
    std::vector<std::string> selections;
    std::string selection;
    
    Data(const std::vector<std::string>& sels, const std::string& sel, const std::string& d)
    : ParserDataBase(d), selections(sels), selection(sel)
    {std::cout<<d<<std::endl;}
    
    virtual std::string cppType(const std::string&) const { return "#error"; }
    virtual std::string cppTypeDecl(const std::string& name) const
    {
      std::ostringstream os;
      os<<"enum "<<cppTypeName(name)<<"\n{"<<endl;
      std::string comma="";
      BOOST_FOREACH(const std::string& s, selections)
      {
	os<<comma<<s<<endl;
	comma=",";
      }
      os<<"};";
      return os.str();
    }
    virtual std::string cppValueRep(const std::string& name) const { return "#error"; }
    
    virtual std::string cppParamType(const std::string& name) const { return "insight::SelectionParameter"; };   

    virtual void cppWriteCreateStatement(std::ostream& os, const std::string& name) const
    {

      os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<";"<<endl;
//       os<<cppParamType(name)<<"& "<<s_fq_name <<" = *value;"<<endl;
      os<<"{"<<endl;
      os<<"insight::SelectionParameter::ItemList items;"<<endl;
      BOOST_FOREACH(const std::string& s, selections)
      {
	os<<"items.push_back(\""<<s<<"\");"<<endl;
      }
      os<<name<<".reset(new "<<cppParamType(name)<<"(\""<< selection <<"\", items, \""<<description<<"\")); "<<endl;
      os<<"}"<<endl;
    }
    
    virtual void cppWriteSetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
      const std::string& thisscope) const
    {
      os<<varname<<"() = int("<< staticname <<");"<<endl;
    }
    
    virtual void cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
      const std::string& thisscope) const
    {
      os<<staticname<<"="<<extendtype(thisscope, cppTypeName(name))<<"("<<varname<<"());"<<endl;
    }
    
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "selection",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( "(" >> *(ruleset.r_identifier) >> ")" >> ruleset.r_identifier >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2, qi::_3)) ]
      ))
    );
  }
};

struct SubsetParameterParser
{
  struct Data
  : public ParserDataBase
  {
    ParameterSetData value;
    
    Data(const ParameterSetData& v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
    virtual void cppAddHeader(std::set< std::string >& headers) const
    {
      BOOST_FOREACH(const ParameterSetEntry& pe, value)
      {
	pe.second->cppAddHeader(headers);
      }
    }
    
    virtual std::string cppType(const std::string&) const { return ""; }
    virtual std::string cppTypeDecl(const std::string& name) const
    {
      std::ostringstream os;
      os<<"struct "<<cppTypeName(name)<<"\n{"<<endl;
      BOOST_FOREACH(const ParameterSetEntry& pe, value)
      {
	pe.second->writeCppHeader(os, pe.first);
      }
      os<<"};";
      return os.str();
    }
    virtual std::string cppValueRep(const std::string& name) const { return "#error"; }
    
    virtual std::string cppParamType(const std::string& name) const { return "insight::SubsetParameter"; };   

    virtual void cppWriteInsertStatement(std::ostream& os, const std::string& psvarname, const std::string& name) const
    {
      os<<"{ ";
      os<<"std::string key(\""<<name<<"\"); ";
      this->cppWriteCreateStatement(os, name);
      os<<"if ("<<psvarname<<".find(key)!="<<psvarname<<".end()) {"<<endl;
      os<<psvarname<<".getSubset(key).merge(*"<<name<<"); ";
      os<<"} else {"<<endl;
      os<<psvarname<<".insert(key, "<<name<<"); ";
      os<<"}"<<endl;
      os<<"}"<<endl;
    }

    virtual void cppWriteCreateStatement(std::ostream& os, const std::string& name) const
    {

      os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<"(new "<<cppParamType(name)<<"(\""<<description<<"\")); "<<endl;      
//       os<<cppParamType(name)<<"& "<<s_fq_name <<" = *value;"<<endl;
      os<<"{"<<endl;
      BOOST_FOREACH(const ParameterSetEntry& pe, value)
      {
	pe.second->cppWriteInsertStatement
	(
	  os, 
	  "(*"+name+")()",
	  pe.first
	);
      }
      os<<"}"<<endl;
    }
    
    virtual void cppWriteSetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
      const std::string& thisscope) const
    {
      std::string myscope=extendtype(thisscope, cppTypeName(name));
      BOOST_FOREACH(const ParameterSetEntry& pe, value)
      {
	std::string subname=pe.first;
	os<<"{"<<endl;
	  os<<pe.second->cppParamType(subname)<<"& "<<subname<<" = "<<varname<<".get< "<<pe.second->cppParamType(subname)<<" >(\""<<subname<<"\");"<<endl;
	  os<<"const "
	    <<extendtype(myscope, pe.second->cppTypeName(subname))
	    <<"& "<<subname<<"_static = "<<staticname<<"."<<subname<<";"<<endl;
	  pe.second->cppWriteSetStatement
	  (
	    os, subname, subname, subname+"_static", myscope
	  );
	os<<"}"<<endl;
      }
    }
    
    virtual void cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
      const std::string& thisscope) const
    {
      std::string myscope=extendtype(thisscope, cppTypeName(name));
      BOOST_FOREACH(const ParameterSetEntry& pe, value)
      {
	std::string subname=pe.first;
	os<<"{"<<endl;
	  os<<"const "<<pe.second->cppParamType(subname)<<"& "<<subname<<" = "<<varname<<".get< "<<pe.second->cppParamType(subname)<<" >(\""<<subname<<"\");"<<endl;
	  os<<extendtype(myscope, pe.second->cppTypeName(subname))
	    <<"& "<<subname<<"_static = "<<staticname<<"."<<subname<<";"<<endl;
	  pe.second->cppWriteGetStatement
	  (
	    os, subname, subname, subname+"_static", myscope
	  );
	os<<"}"<<endl;
      }
    }
    
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "set",
     typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
      ( "{" > ruleset.r_parameterset > "}" >> ruleset.r_description_string ) 
       [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
     ))
    );
  }
};

struct SelectableSubsetParameterParser
{
  struct Data
  : public ParserDataBase
  {
    typedef boost::fusion::vector2<std::string, ParserDataBase::Ptr> SubsetData;
    typedef std::vector<SubsetData> SubsetListData;
    
    std::string default_sel;
    SubsetListData value;
    
    Data(const SubsetListData& v, const std::string& ds, const std::string& d)
    : ParserDataBase(d), default_sel(ds), value(v)
    {}
    
    virtual void cppAddHeader(std::set<std::string>& headers) const 
    {
      headers.insert("<boost/variant.hpp>");
      BOOST_FOREACH(const SubsetData& pe, value)
      {
	boost::fusion::get<1>(pe)->cppAddHeader(headers);
      }
    };
    
    virtual std::string cppType(const std::string& name) const
    {
      std::ostringstream os;
      os<<"boost::variant<";
      std::string comma="";
      BOOST_FOREACH(const SubsetData& pe, value)
      {
	os<<comma<< (name+"_"+boost::fusion::get<0>(pe)+"_type");
	comma=",";
      }
      os<<">";
      return os.str();
    }
    virtual std::string cppValueRep(const std::string& name) const { return "#error"; }
    
    virtual std::string cppTypeDecl(const std::string& name) const
    {
      std::ostringstream os;
      BOOST_FOREACH(const SubsetData& pe, value)
      {
	std::string tname=(name+"_"+boost::fusion::get<0>(pe));
	ParserDataBase::Ptr pd=boost::fusion::get<1>(pe);
	os<<pd->cppTypeDecl(tname)<<endl;
      } 
      return 
	os.str()+"\n"
	+ParserDataBase::cppTypeDecl(name);
    }

    virtual std::string cppParamType(const std::string& name) const { return "insight::SelectableSubsetParameter"; };   

    virtual void cppWriteCreateStatement(std::ostream& os, const std::string& name) const
    {
     
      os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<";"<<endl;
      os<<"{"<<endl;
      os<<"insight::SelectableSubsetParameter::SubsetList "<<name<<"_selection;"<<endl;
      BOOST_FOREACH(const SubsetData& sd, value)
      {
	const std::string& sel_name=boost::fusion::get<0>(sd);
	ParserDataBase::Ptr pd=boost::fusion::get<1>(sd); // should be a set
	
	os<<"{"<<endl;
	pd->cppWriteCreateStatement
	(
	  os, sel_name
	);
	os<<name<<"_selection.push_back(insight::SelectableSubsetParameter::SingleSubset(\""<<sel_name<<"\", "<<sel_name<<".release()));"<<endl;
	os<<"}"<<endl;
      }
      os<<name<<".reset(new "<<cppParamType(name)<<"(\""<<default_sel<<"\", "<<name<<"_selection, \""<<description<<"\")); "<<endl;      
      os<<"}"<<endl;
    }
    
    virtual void cppWriteSetStatement
    (
      std::ostream& os, 
      const std::string& name, 
      const std::string& varname, 
      const std::string& staticname, 
      const std::string& thisscope
    ) const
    {

      os<<"{"<<endl;
      
      BOOST_FOREACH(const SubsetData& sd, value)
      {
	const std::string& sel_name=boost::fusion::get<0>(sd);
	ParserDataBase::Ptr pd=boost::fusion::get<1>(sd); // should be a set
	std::string seliname=name+"_"+sel_name;
	os<<"if ( ";
	os<<"const "
	<<extendtype(thisscope, pd->cppTypeName(name+"_"+sel_name))
	<<"* "<<seliname<<"_static = boost::get< "<<extendtype(thisscope, pd->cppTypeName(name+"_"+sel_name))<<" >(&"<< staticname <<")"
	<<") {"<<endl;
	os<<varname<<".selection() = \""<<sel_name<<"\";"<<endl;
	os<<"ParameterSet& "<<seliname<<"_param = "<<name<<"();"<<endl;
	pd->cppWriteSetStatement(os, seliname, seliname+"_param", "(*"+seliname+"_static)", thisscope);
	os<<"}"<<endl;
      }
      os<<"}"<<endl;
    }

    virtual void cppWriteGetStatement
    (
      std::ostream& os, 
      const std::string& name, 
      const std::string& varname, 
      const std::string& staticname, 
      const std::string& thisscope
    ) const
    {

      os<<"{"<<endl;
      
      BOOST_FOREACH(const SubsetData& sd, value)
      {
	const std::string& sel_name=boost::fusion::get<0>(sd);
	ParserDataBase::Ptr pd=boost::fusion::get<1>(sd); // should be a set
	std::string seliname=name+"_"+sel_name;
	
	os<<"if ( "<<varname<<".selection() == \""<<sel_name<<"\" ) {"<<endl;
	
	os<<"const ParameterSet& "<<seliname<<"_param = "<<name<<"();"<<endl;
	
// 	os<<extendtype(thisscope, pd->cppTypeName(name+"_"+sel_name))
// 	<<"& "<<seliname<<"_static = boost::get< "<<extendtype(thisscope, pd->cppTypeName(name+"_"+sel_name))<<" >("<< staticname <<");"<<endl;
	os<<extendtype(thisscope, pd->cppTypeName(name+"_"+sel_name))<<" "<<seliname<<"_static;";
	
	pd->cppWriteGetStatement(os, seliname, seliname+"_param", seliname+"_static", thisscope);
	os<<staticname<<" = "<<seliname<<"_static;";
	
	os<<"}"<<endl;
      }
      os<<"}"<<endl;
    }
  };
  
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "selectablesubset",
     typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
      ( lit("{{") >> 
	  *(ruleset.r_identifier >> ruleset.r_parameterdata) 
	>> lit("}}") >> ruleset.r_identifier >> ruleset.r_description_string ) 
       [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2, qi::_3)) ]
     ))
    );
  }
};


struct ArrayParameterParser
{
  struct Data
  : public ParserDataBase
  {
    ParserDataBase::Ptr value;
    int num;
    
    Data(ParserDataBase::Ptr v, int n, const std::string& d)
    : ParserDataBase(d), value(v), num(n)
    {std::cout<<d<<std::endl;}
    
    virtual void cppAddHeader(std::set<std::string>& headers) const 
    {
      headers.insert("<vector>");
      value->cppAddHeader(headers);
    };

    virtual std::string cppValueRep(const std::string& name) const { return "#error"; }

    virtual std::string cppType(const std::string& name) const
    {
      return "std::vector<"+value->cppTypeName(name+"_default")+">";
    }
    
    virtual std::string cppTypeDecl(const std::string& name) const
    {
      return 
	value->cppTypeDecl(name+"_default")
	+"\n"
	+ParserDataBase::cppTypeDecl(name);
    }

    virtual std::string cppParamType(const std::string& name) const { return "insight::ArrayParameter"; };   

    virtual void cppWriteCreateStatement(std::ostream& os, const std::string& name) const
    {

      os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<"(new "<<cppParamType(name)<<"(\""<<description<<"\")); "<<endl;      
      
      os<<"{"<<endl;
      value->cppWriteCreateStatement
      (
	os, name+"_default_value"
      );
      os<<name<<"->setDefaultValue(*"<<name<<"_default_value.release());"<<endl;
      os<<"for (int i=0; i<"<<num<<"; i++) "<<name<<"->appendEmpty();"<<endl;
      os<<"}"<<endl;
    }
    
    virtual void cppWriteSetStatement(std::ostream& os, const std::string& name, const std::string& varname, 
				      const std::string& staticname, const std::string& thisscope) const
    {
      os<<varname<<".clear();"<<endl;
      os<<"for(size_t k=0; k<"<<staticname<<".size(); k++)"<<endl;
      os<<"{"<<endl;
      os<<varname<<".appendEmpty();"<<endl;
      
      os<<value->cppParamType(name+"_default")<<"& "<<varname
	<<"_cur = dynamic_cast< "<< value->cppParamType(name+"_default") <<"& >("<<varname<<"[k]);"<<endl;
      os<<"const "<<extendtype(thisscope, value->cppTypeName(name+"_default"))<<"& "<<varname<<"_cur_static = "<<staticname<<"[k];"<<endl;
      
      value->cppWriteSetStatement(os, name+"_default", name+"_cur", name+"_cur_static", thisscope);
      
      os<<"}"<<endl;
    }
    
    virtual void cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, 
				      const std::string& staticname, const std::string& thisscope) const
    {
      os<<staticname<<".resize("<<varname<<".size());"<<endl;
      os<<"for(int k=0; k<"<<varname<<".size(); k++)"<<endl;
      os<<"{"<<endl;
      os<<"const "<<value->cppParamType(name+"_default")<<"& "<<varname
	<<"_cur = dynamic_cast<const "<< value->cppParamType(name+"_default") <<"& >("<<varname<<"[k]);"<<endl;
      os<<extendtype(thisscope, value->cppTypeName(name+"_default"))<<"& "<<varname<<"_cur_static = "<<staticname<<"[k];"<<endl;
      
      value->cppWriteGetStatement(os, name+"_default", name+"_cur", name+"_cur_static", thisscope);
      
      os<<"}"<<endl;
    }
    
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "array",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( '[' >> ruleset.r_parameterdata >> ']' >> '*' >> int_ >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2, qi::_3)) ]
      ))	
    );
  }
};



struct MatrixParameterParser
{
  struct Data
  : public ParserDataBase
  {
    arma::mat value;
    
    Data(int r, int c, const std::string& d)
    : ParserDataBase(d), value(arma::zeros(r,c))
    {}
    
    virtual void cppAddHeader(std::set<std::string>& headers) const 
    {
      headers.insert("<armadillo>");
    };
    
    virtual std::string cppType(const std::string&) const
    {
      return "arma::mat";
    }

    virtual std::string cppParamType(const std::string& name) const 
    { 
      return "insight::MatrixParameter";
    }; 
    
    virtual std::string cppValueRep(const std::string& name) const 
    { 
      return "#error";
    }
    
    virtual void cppWriteCreateStatement(std::ostream& os, const std::string& name) const
    {

      os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<";"<<endl;
//       os<<cppParamType(name)<<"& "<<s_fq_name <<" = *value;"<<endl;
      os<<"{"<<endl;
      os<<"arma::mat data; data"<<endl;
      for (int i=0; i<value.n_rows;i++)
      {
	for (int j=0; j<value.n_cols;j++)
	{
	  os<<"<<"<<value(i,j)<<endl;
	}
	os<<"<<arma::endr";
      };
      os<<";"<<endl;
      os<<name<<".reset(new "<<cppParamType(name)<<"(data, \""<<description<<"\")); "<<endl;
      os<<"}"<<endl;
    }
    
  };
  
  template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "matrix",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( qi::int_ >> 'x' >> qi::int_ >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(
           new_<Data>(qi::_1, qi::_2, qi::_3)
          ) ]
      ))
    );
  }
};



template <typename Iterator, typename Skipper>
struct PDLParser
: qi::grammar< Iterator, ParameterSetData(), Skipper >
{

public:
  
  PDLParserRuleset<Iterator,Skipper> rules;
    
  PDLParser()
  : PDLParser::base_type(rules.r_parameterset)
  {
    BoolParameterParser::insertrule<Iterator, Skipper>(rules);
    DoubleParameterParser::insertrule<Iterator, Skipper>(rules);
    VectorParameterParser::insertrule<Iterator, Skipper>(rules);
    StringParameterParser::insertrule<Iterator, Skipper>(rules);
    PathParameterParser::insertrule<Iterator, Skipper>(rules);
    IntParameterParser::insertrule<Iterator, Skipper>(rules);
    SubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    SelectionParameterParser::insertrule<Iterator, Skipper>(rules);
    ArrayParameterParser::insertrule<Iterator, Skipper>(rules);
    SelectableSubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    MatrixParameterParser::insertrule<Iterator, Skipper>(rules);
    
    rules.init();

    on_error<fail>(rules.r_parameterset,
		      phx::ref(std::cout)
		      << "Error! Expecting "
		      << qi::_4
		      << " here: '"
		      << phx::construct<std::string>(qi::_3, qi::_2)
		      << "'\n"
		    );
  }
    
};



int main(int argc, char *argv[])
{
  for (int k=1; k<argc; k++)
  {
    boost::filesystem::path inf(argv[k]);
    cout<<"Processing PDL "<<inf<<endl;
    
    std::ifstream in(inf.c_str());
    
    if (!in.good()) exit(-1);

    PDLParser<std::string::iterator> parser;
    skip_grammar<std::string::iterator> skip;
    
    std::string first_word, base_type_name="";
    in>>first_word;
    if (first_word=="inherits")
    {
      in>>base_type_name;
    }
    else
    {
      in.seekg(0, std::ios::beg);
    }
      
    std::istreambuf_iterator<char> eos;
    std::string contents_raw(std::istreambuf_iterator<char>(in), eos);
//     std::string contents_raw;
//     in.seekg(0, std::ios::end);
//     contents_raw.resize(in.tellg());
//     in.seekg(0, std::ios::beg);
//     in.read(&contents_raw[0], contents_raw.size());
    
    std::string::iterator first=contents_raw.begin();
    std::string::iterator last=contents_raw.end();
    
    ParameterSetData result;
    bool r = qi::phrase_parse
    (
	first, 
	last,
	parser,
	skip,
	result
    );
    
    {
      std::string bname=inf.stem().string();
      std::vector<std::string> parts;
      boost::algorithm::split_regex(parts, bname, boost::regex("__") );
      std::string name=bname;
      if (parts.size()==3)
	name=parts[2];
      
      
      {
	std::ofstream f(bname+"_headers.h");
	std::set<std::string> headers;
	BOOST_FOREACH(const ParameterSetEntry& pe, result)
	{
	  pe.second->cppAddHeader(headers);
	}
  //       result->cppAddHeader(headers);
	BOOST_FOREACH(const std::string& h, headers)
	{
	  f<<"#include "<<h<<endl;
	}
      }
      {
	std::ofstream f(bname+".h");
	
	f<<"struct "<<name<<endl;
	if (!base_type_name.empty())
	  f<<": public "<<base_type_name<<endl;
	f<<"{"<<endl;
	
	// declare variables and types
	BOOST_FOREACH(const ParameterSetEntry& pe, result)
	{
	  pe.second->writeCppHeader(f, pe.first);
	}
	
	f
	<<name<<"()"<<endl;
	if (!base_type_name.empty())
	  f<<" : "<<base_type_name<<"()"<<endl;
	f
	<<"{"<<endl;
	f<<"}"<<endl
	;
	
	//get from other ParameterSet
	f
	<<name<<"(const insight::ParameterSet& p)"<<endl;
	if (!base_type_name.empty())
	  f<<" : "<<base_type_name<<"(p)"<<endl;
	f
	<<"{"<<endl
	<<" get(p);"<<endl
	<<"}"<<endl
	;
	
	//set into other ParameterSet
	f
	<<"void set(insight::ParameterSet& p) const"<<endl
	<<"{"<<endl;
	if (!base_type_name.empty())
	  f<<" "<<base_type_name<<"::set(p);"<<endl;
	BOOST_FOREACH(const ParameterSetEntry& pe, result)
	{
	  std::string subname=pe.first;
	  f<<"{"<<endl;
	  f<<pe.second->cppParamType(subname)<<"& "<<subname<<" = p.get< "<<pe.second->cppParamType(subname)<<" >(\""<<subname<<"\");"<<endl;
	  f<<"const "<<pe.second->cppTypeName(subname)<<"& "<<subname<<"_static = this->"<<subname<<";"<<endl;
	  pe.second->cppWriteSetStatement
	  (
	    f, subname, subname, subname+"_static", ""
	  );
	  f<<"}"<<endl;
	}
	f
	<<"}"<<endl
	;
	
	//from other ParameterSet into current static data
	f
	<<"void get(const insight::ParameterSet& p)"<<endl
	<<"{"<<endl;
	if (!base_type_name.empty())
	  f<<" "<<base_type_name<<"::get(p);"<<endl;
	BOOST_FOREACH(const ParameterSetEntry& pe, result)
	{
	  std::string subname=pe.first;
	  f<<"{"<<endl;
	  f<<"const "<<pe.second->cppParamType(subname)<<"& "<<subname<<" = p.get< "<<pe.second->cppParamType(subname)<<" >(\""<<subname<<"\");"<<endl;
	  f<<pe.second->cppTypeName(subname)<<"& "<<subname<<"_static = this->"<<subname<<";"<<endl;
	  pe.second->cppWriteGetStatement
	  (
	    f, subname, subname, subname+"_static", ""
	  );
	  f<<"}"<<endl;
	}
	f
	<<"}"<<endl
	;
	
	// create a ParameterSet with default values set
	f<<"static ParameterSet makeDefault() {"<<endl;
	f<<"ParameterSet p;"<<endl;
	if (!base_type_name.empty())
	  f<<" p="<<base_type_name<<"::makeDefault();"<<endl;
	BOOST_FOREACH(const ParameterSetEntry& pe, result)
	{
	  pe.second->cppWriteInsertStatement
	  (
	    f, 
	    "p",
	    pe.first
	  );
	}      
	f<<"return p;"<<endl<<"}"<<endl;
	
	// convert static data into a ParameterSet
	f<<"operator ParameterSet() const"<<endl;
	f<<"{ ParameterSet p=makeDefault(); set(p); return p; }"<<endl;
	
	f
	<<"};"<<endl;
      }
    }
  }
  return 0;
}
