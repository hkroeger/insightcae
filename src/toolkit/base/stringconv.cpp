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


#include "stringconv.h"

#include <sstream>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace insight
{


template<>
std::string toString(const arma::mat&value)
{
    std::string s;
    for (arma::uword i=0; i<value.n_elem; i++)
    {
        if (i>0) s+=" ";
        s += toString<double>(value(i));
    }
    return s;
}

template<>
std::string toString(const std::string& s)
{
    return s;
}

template<>
std::string toString(const boost::gregorian::date &date)
{
    return boost::gregorian::to_simple_string(date);
}

template<>
std::string toString(const boost::posix_time::ptime &datetime)
{
    return boost::posix_time::to_simple_string(datetime);
}


namespace {

template <typename T>
bool is_only_a(const std::string& str)
{
    std::stringstream ss(str);
    T x;
    return (ss >> x && ss.rdbuf()->in_avail() ==0);
}

}

bool isNumber(const string &s)
{
    return
       is_only_a<unsigned long>(s)
    || is_only_a<int>(s)
    || is_only_a<float>(s);

  // try {
  //   auto result = boost::lexical_cast<double>(s);
  //   return true;
  // }
  // catch (const boost::bad_lexical_cast&)
  // {
  //   return false;
  //   }
}


template<>
arma::mat toValue(const std::string& s)
{
    CurrentExceptionContext ex(insight::VerbosityLevel::Loops, "converting string \""+s+"\" into vector", false);

    std::vector<std::string> cmpts;
    auto st = boost::trim_copy(s);
    boost::split(
        cmpts,
        st,
        boost::is_any_of(" \t\n,;"),
        token_compress_on
        );
    std::vector<double> vals;
    for (size_t i=0; i<cmpts.size(); i++)
    {
        vals.push_back( toNumber<double>(cmpts[i]) );
    }

    return arma::mat(vals.data(), vals.size(), 1);
}

template<>
std::string toValue(const std::string& s)
{
    return s;
}

template<>
boost::gregorian::date toValue(const std::string& s)
{
    return boost::gregorian::from_simple_string(s);
}


template<>
boost::posix_time::ptime toValue(const std::string& s)
{
    return boost::posix_time::time_from_string(s);
}


}
