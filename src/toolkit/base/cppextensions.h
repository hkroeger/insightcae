#ifndef CPPEXTENSIONS_H
#define CPPEXTENSIONS_H
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
 * This header defines several extensions to C++ and Boost. It is kept as a
 * backward-compatible umbrella over base/observer_ptr.h and
 * base/stl_container_extensions.h. ObjectWithBoostSignalConnections moved to
 * base/objectwithboostsignalconnections.h and is *not* re-included here, so
 * that changes to it (and its boost::signals2 dependency) don't force a
 * rebuild of every consumer of this header.
 *
 */

#include "base/exception.h"
#include "base/factory.h"
#include "base/observer_ptr.h"
#include "base/stl_container_extensions.h"

#endif // CPPEXTENSIONS_H
