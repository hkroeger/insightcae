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
#define BOOST_SPIRIT_DEBUG

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

using namespace std;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;
using namespace boost;


/* Basic data structures */
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

    virtual void cppWriteSetStatement(std::ostream& os, 
				      const std::string& psvarname,
				      std::vector<std::string> prefixes, 
				      const std::string& name, const std::string& sbase="(*this)") const
    {
      prefixes.push_back(name);
      std::string 
       p_fq_name=boost::algorithm::join(prefixes, "/"),
       s_fq_name=boost::algorithm::join(prefixes, ".");
      os<<psvarname<<".get< "<<cppParamType(name)<<" > (\""<<name<<"\")() = "<<sbase<<"."<<s_fq_name<<";"<<endl;
    }
    
};

typedef std::pair<std::string, ParserDataBase::Ptr> ParameterSetEntry;
typedef std::vector< ParameterSetEntry > ParameterSetData;

template <typename Iterator, typename Skipper = qi::space_type >
struct PDLParserRuleset
{
  typedef qi::rule<Iterator, ParserDataBase::Ptr(), Skipper> ParameterDataRule;
  typedef boost::shared_ptr<ParameterDataRule> ParameterDataRulePtr;

  qi::rule<Iterator, std::string(), Skipper> r_identifier, r_string, r_description_string;
  qi::rule<Iterator, ParameterSetData(), Skipper> r_parameterset;
  qi::rule<Iterator, ParameterSetEntry(), Skipper> r_parametersetentry;
  
  qi::symbols<char, ParameterDataRulePtr> parameterDataRules;
  qi::rule<Iterator, ParserDataBase::Ptr(), Skipper, qi::locals<ParameterDataRulePtr> > r_parameterdata;
  
  PDLParserRuleset()
  {  
    r_string = as_string[ lexeme [ "\"" >> *~char_("\"") >> "\"" ] ];
    r_description_string = (r_string | attr(""));
    r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
    
    r_parameterdata %= omit[ parameterDataRules[ qi::_a = qi::_1 ] ] >> qi::lazy(*qi::_a);
    
    
    r_parametersetentry = r_identifier >> '=' >> r_parameterdata;
    r_parameterset = *( r_parametersetentry );
    
    BOOST_SPIRIT_DEBUG_NODE(r_identifier);
    BOOST_SPIRIT_DEBUG_NODE(r_parameterdata);
    BOOST_SPIRIT_DEBUG_NODE(r_parameterset);
    BOOST_SPIRIT_DEBUG_NODE(r_parametersetentry);
    cout<<"ok1"<<endl;
  }
  
  void init()
  {
  }
  
};

/* specialized data structures */
struct DoubleParameterParser
{
  struct Data
  : public ParserDataBase
  {
    double value;
    
    Data(double v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
    virtual std::string cppType(const std::string&) const
    {
      return "double";
    }
    virtual std::string cppParamType(const std::string& name) const { return "insight::DoubleParameter"; };   
    virtual std::string cppValueRep(const std::string& name) const { return boost::lexical_cast<std::string>(value); }
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
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
      os<<".convert_to_container<std::vector<double> >())";
      return os.str();
    }
    
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "vector",
      typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
	( "(" >> *double_ >> ")" >> ruleset.r_description_string ) 
	[ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(phx::construct<arma::mat>(qi::_1), qi::_2)) ]
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
  
  template <typename Iterator, typename Skipper = qi::space_type >
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
  
  template <typename Iterator, typename Skipper = qi::space_type >
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
    
    virtual void cppWriteSetStatement(std::ostream& os, const std::string& psvarname, 
				      std::vector< std::string > prefixes, 
				      const std::string& name, const std::string& sbase="(*this)") const
    {
      prefixes.push_back(name);
      std::string 
       p_fq_name=boost::algorithm::join(prefixes, "/"),
       fq_name=boost::algorithm::join(prefixes, "."),
       s_fq_name=boost::algorithm::join(prefixes, "_");
      os
	<<cppParamType(name)<<"& "<<s_fq_name
	<<" = "<<psvarname<<".get< "<<cppParamType(name)<<" > (\""<<name<<"\");"<<endl;
      os<<"{"<<endl;
      BOOST_FOREACH(const ParameterSetEntry& pe, value)
      {
	pe.second->cppWriteSetStatement
	(
	  os, 
	  s_fq_name,
	  prefixes, 
	  pe.first
	);
      }
      os<<"}"<<endl;
    }
    
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
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
    
    SubsetListData value;
    
    Data(const SubsetListData& v, const std::string& d)
    : ParserDataBase(d), value(v)
    {std::cout<<d<<std::endl;}
    
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
     
      os<<"{"<<endl;
      BOOST_FOREACH(const SubsetData& sd, value)
      {
	os<<"SubsetList "<<name<<"_selection;"<<endl;
	
	os<<"{"<<endl;
	const std::string& sel_name=boost::fusion::get<0>(sd);
	ParserDataBase::Ptr pd=boost::fusion::get<1>(sd); // should be a set
	pd->cppWriteCreateStatement
	(
	  os, sel_name
	);
	os<<name<<"_selection.push_back(SingleSubset(\""<<sel_name<<"\", "<<sel_name<<".release()));"<<endl;
	os<<"}"<<endl;
      }
      os<<"std::auto_ptr< "<<cppParamType(name)<<" > value(new "<<cppParamType(name)<<"(\""<<description<<"\")); "<<endl;      
      os<<"}"<<endl;
    }
    
    virtual void cppWriteSetStatement(std::ostream& os, const std::string& psvarname, 
				      std::vector< std::string > prefixes, 
				      const std::string& name, const std::string& sbase="(*this)") const
    {
//       prefixes.push_back(name);
//       std::string 
//        p_fq_name=boost::algorithm::join(prefixes, "/"),
//        fq_name=boost::algorithm::join(prefixes, "."),
//        s_fq_name=boost::algorithm::join(prefixes, "_");
//       os
// 	<<cppParamType(name)<<"& "<<s_fq_name
// 	<<" = "<<psvarname<<".get< "<<cppParamType(name)<<" > (\""<<name<<"\");"<<endl;
//       os<<"{"<<endl;
//       BOOST_FOREACH(const SubsetData& pe, value)
//       {
// 	pe.second->cppWriteSetStatement
// 	(
// 	  os, 
// 	  s_fq_name,
// 	  prefixes, 
// 	  pe.first
// 	);
//       }
//       os<<"}"<<endl;
    }
    
  };
  
  
  template <typename Iterator, typename Skipper = qi::space_type >
  inline static void insertrule(PDLParserRuleset<Iterator,Skipper>& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      "selectablesubset",
     typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
      ( lit("{{") >> 
	  *(ruleset.r_identifier >> ruleset.r_parameterdata) 
	>> lit("}}") >> ruleset.r_description_string ) 
       [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(qi::_1, qi::_2)) ]
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
    
    virtual void cppWriteSetStatement(
      std::ostream& os, 
      const std::string& psvarname, 
      std::vector< std::string > prefixes, const std::string& name, const std::string& sbase = "(*this)") const
      {
      }
    
  };
  
  template <typename Iterator, typename Skipper = qi::space_type >
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

template <typename Iterator, typename Skipper = qi::space_type >
struct PDLParser
: qi::grammar< Iterator, ParameterSetData(), Skipper >
{

public:
  
  PDLParserRuleset<Iterator,Skipper> rules;
    
  PDLParser()
  : PDLParser::base_type(rules.r_parameterset)
  {
    DoubleParameterParser::insertrule<Iterator, Skipper>(rules);
    VectorParameterParser::insertrule<Iterator, Skipper>(rules);
    PathParameterParser::insertrule<Iterator, Skipper>(rules);
    IntParameterParser::insertrule<Iterator, Skipper>(rules);
    SubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    ArrayParameterParser::insertrule<Iterator, Skipper>(rules);
    SelectableSubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    cout<<"ok2"<<endl;
    
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
  boost::filesystem::path inf(argv[1]);
  std::ifstream in(inf.c_str());

  PDLParser<std::string::iterator> parser;
  //   skip_grammar<Iterator> skip;
    
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  cout<<contents_raw<<endl;
  
  std::string::iterator first=contents_raw.begin();
  std::string::iterator last=contents_raw.end();
  
  ParameterSetData result;
  bool r = qi::phrase_parse
  (
      first, 
      last,
      parser,
      qi::space,
      result
  );
  
  cout<<"Parsing done"<<endl;
  
  {
    std::string name=inf.stem().string();
    
    
    {
      std::ofstream f(name+"_headers.h");
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
      std::ofstream f(name+".h");
      
      f<<"struct "<<name<<" : public insight::ParameterSet"<<endl;
      f<<"{"<<endl;
      
      BOOST_FOREACH(const ParameterSetEntry& pe, result)
      {
	pe.second->writeCppHeader(f, pe.first);
      }
      
      f
      <<name<<"()"<<endl
      <<"{"<<endl;
      f<<"}"<<endl
      ;
      
      //get from other ParameterSet
      f
      <<name<<"(const insight::ParameterSet& p)"<<endl
      <<"{ }"<<endl
      ;
      
      //set into other ParameterSet
      f
      <<"void set(insight::ParameterSet& p)"<<endl
      <<"{"<<endl;
      BOOST_FOREACH(const ParameterSetEntry& pe, result)
      {
	pe.second->cppWriteSetStatement
	(
	  f, 
	  "p",
	  std::vector<std::string>(), 
	  pe.first
	);
      }
      f
      <<"}"<<endl
      ;
      
      f<<"static ParameterSet makeDefault() {"<<endl;
      f<<"ParameterSet p;"<<endl;
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
      f
      <<"};"<<endl;
    }
  }
  
  return 0;
}