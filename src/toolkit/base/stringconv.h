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


#ifndef INSIGHT_STRINGCONV_H
#define INSIGHT_STRINGCONV_H

#include <string>
#include <sstream>
#include <locale>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "base/exception.h"
#include "base/linearalgebra.h"
#include "base/stl_container_extensions.h"

namespace insight {

template<class V>
std::string toString(const V& value)
{
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(12);
    os << value;
    return os.str();
}

template<>
std::string toString(const std::string& value);

template<>
std::string toString(const arma::mat& value);

template<>
std::string toString(const boost::gregorian::date& value);

template<>
std::string toString(const boost::posix_time::ptime& value);


template<class T = double>
T toNumber(const std::string& s)
{
    try
    {
        return boost::lexical_cast<T>(
            boost::algorithm::trim_copy(s) );
    }
    catch (const boost::bad_lexical_cast& e)
    {
        throw insight::Exception("expected a number, got \""+s+"\"");
    }
}

bool isNumber(const std::string& s);


template<class Container>
std::string toStringList(
    const Container& vals,
    const std::string& sep = "; " )
{
    std::vector<std::string> strVals;
    std::transform(
        vals.begin(), vals.end(),
        std::back_inserter(strVals),
        &toString<double>
        );
    return boost::join(strVals, sep);
}


template<class Container>
Container toNumberList(
    const std::string& listStr,
    const std::string& sep = "; " )
{
    std::vector<std::string> strVals;
    boost::split(
        strVals, listStr,
        boost::is_any_of(sep),
        boost::algorithm::token_compress_on);

    Container vals;
    std::transform(
        strVals.begin(), strVals.end(),
        std::last_inserter<Container>(vals),
        &toNumber<double>
        );
    return vals;
}


template<class V>
V toValue(const std::string& s)
{
    return toNumber<V>(s);
}

template<>
std::string toValue(const std::string& s);

template<>
arma::mat toValue(const std::string& s);

template<>
boost::gregorian::date toValue(const std::string& s);

template<>
boost::posix_time::ptime toValue(const std::string& s);

}

#endif // INSIGHT_STRINGCONV_H
