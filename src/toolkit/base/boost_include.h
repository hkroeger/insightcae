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


#ifndef INSIGHT_BOOST_INCLUDE_H
#define INSIGHT_BOOST_INCLUDE_H


#define BOOST_SPIRIT_USE_PHOENIX_V3

// #define SPIRIT_ARGUMENTS_LIMIT 20
// #define BOOST_PHOENIX_LIMIT 20
// #define PHOENIX_LIMIT 20
// #define FUSION_MAX_VECTOR_SIZE 20

// #define SPIRIT_ARGUMENTS_LIMIT 20
// #define BOOST_SPIRIT_ARGUMENTS_LIMIT 20
// #define FUSION_MAX_VECTOR_SIZE 20
// #define PHOENIX_ARG_LIMIT 20
// #define PHOENIX_ACTOR_LIMIT 20
// #define BOOST_PHOENIX_ACTOR_LIMIT 20
// #define PHOENIX_LIMIT 20
// #define BOOST_PHOENIX_LIMIT 20
// #define BOOST_PHOENIX_ARG_LIMIT 20
// #define BOOST_PHOENIX_COMPOSITE_LIMIT 20
// #define BOOST_PHOENIX_CONSTRUCT_LIMIT 20
// #define PHOENIX_CONSTRUCT_LIMIT 20
// #define PHOENIX_COMPOSITE_LIMIT 20

#ifndef Q_MOC_RUN

#include "boost/version.hpp"

#ifndef WIN32
#define BOOST_NO_SCOPED_ENUMS
#define BOOST_NO_CXX11_SCOPED_ENUMS
#endif

#ifdef WIN32
#include "winsock2.h"
#include "windows.h"
#endif

#include "boost/asio.hpp"

#include "boost/filesystem.hpp"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

//#include "boost/assign.hpp"
#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>

#include "boost/format.hpp"
#include <boost/tokenizer.hpp>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/algorithm/string.hpp>
#include "boost/date_time.hpp"

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
#include "boost/function.hpp"

#include "boost/timer/timer.hpp"

#include "boost/range/adaptor/indexed.hpp"
#include "boost/phoenix.hpp"

#include <functional>
#endif

typedef boost::filesystem::path bfs_path;

namespace boost
{
namespace filesystem
{

template < >
path& path::append< typename path::iterator >(
    typename path::iterator begin,
    typename path::iterator end,
    const codecvt_type& cvt );



boost::filesystem::path
make_relative(
    boost::filesystem::path a_From,
    boost::filesystem::path a_To );

bool path_contains_file(
    path dir,
    path file );


bool weakly_equivalent(
    path first,
    path second );

bool is_executable(const boost::filesystem::path& p);

}

namespace phoenix {
template <typename T>
struct make_shared_f
{
    template <typename... A> struct result
    { typedef std::shared_ptr<T> type; };

    template <typename... A>
    typename result<A...>::type operator()(A&&... a) const {
        return std::make_shared<T>(std::forward<A>(a)...);
    }
};

template <typename T>
using make_shared_ = boost::phoenix::function<make_shared_f<T> >;
}

}

namespace std
{

template<> struct hash<boost::filesystem::path>
{
    std::size_t operator()(const boost::filesystem::path& fn) const;
};


}



#endif
