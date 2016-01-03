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

#ifndef INSIGHT_MAPKEY_PARSER_H
#define INSIGHT_MAPKEY_PARSER_H

#include "base/boost_include.h"
#include "base/exception.h"
#ifndef Q_MOC_RUN
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>
#endif
#include "cadtypes.h"

namespace mapkey_parser
{
  BOOST_SPIRIT_TERMINAL(mapkey);
  
//   namespace tag { struct mapkey{}; } // tag identifying placeholder
//   typedef unspecified<tag::mapkey> mapkey_type;
//   mapkey_type const mapkey = {};   // placeholder itself
}

namespace boost { namespace spirit
{
    // We want custom_parser::iter_pos to be usable as a terminal only,
    // and only for parser expressions (qi::domain).
    template <>
    struct use_terminal<qi::domain, mapkey_parser::tag::mapkey>
      : mpl::true_
    {};
}}

namespace mapkey_parser
{
    template<class T>
    struct mapkey_parser
      : boost::spirit::qi::primitive_parser<mapkey_parser<T> >
    {
	
	const std::map<std::string, T>* map_;
	
	mapkey_parser()
	: map_(NULL)
	{}
	
	mapkey_parser(const std::map<std::string, T>& map)
	: map_(&map)
	{}
	
        // Define the attribute type exposed by this parser component
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef std::string type;
        };
 
        // This function is called during the actual parsing process
        template <typename Iterator, typename Context, typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last, Context&, Skipper const& skipper, Attribute& attr) const
        {
	  if (!map_)
	    throw insight::Exception("Attempt to use unallocated map parser wrapper!");
	  
            boost::spirit::qi::skip_over(first, last, skipper);
	    
	    Iterator cur=first, match=first;
	    bool matched=false;
// 	    cur++;
	    while (cur!=last)
	    {
	      std::string key(first, cur);

	      typename std::map<std::string, T>::const_iterator it=map_->find(key);
	      if (it!=map_->end())
	      {
//   		  std::cout<<"MATCH=>"<<key<<"<"<<std::endl;
		  match=cur;
		  matched=true;
	      }
//  	      else std::cout<<"NOK=>"<<key<<"<"<<std::endl;
	      cur++;
	    }
	    
	    if (matched)
	    {
	      std::string key(first, match);
	      boost::spirit::traits::assign_to(std::string(first, match), attr);
	      first=match;
//  	      std::cout<<"OK! >"<<key<<"<"<<std::endl;
	      return true;
	    }
	    else
	    {
// 	      std::cout<<"NOK!"<<std::endl;
	      return false;
	    }
        }
        
 
        // This function is called during error handling to create
        // a human readable string for the error context.
        template <typename Context>
        boost::spirit::info what(Context&) const
        {
            return boost::spirit::info("mapkey");
        }
    };
}

#endif