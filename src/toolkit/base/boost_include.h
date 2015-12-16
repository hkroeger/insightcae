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
#if ( BOOST_VERSION < 105100 )
#define BOOST_NO_SCOPED_ENUMS
#else
#define BOOST_NO_CXX11_SCOPED_ENUMS
#endif

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

#endif

#endif
