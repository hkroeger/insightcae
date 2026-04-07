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

#include "boost/version.hpp"

#ifndef Q_MOC_RUN

#ifdef WIN32
#include "winsock2.h"
#include "windows.h"
#endif

#include "boost/filesystem.hpp"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

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



bool startsWith(const path& p, const path& prefix);




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

}



namespace std
{

template<> struct hash<boost::filesystem::path>
{
    std::size_t operator()(const boost::filesystem::path& fn) const;
};


}



#endif
