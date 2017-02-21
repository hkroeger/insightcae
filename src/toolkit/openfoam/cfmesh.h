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

#ifndef INSIGHT_CFMESH_H
#define INSIGHT_CFMESH_H

#include <string>
#include <vector>

#include "openfoam/snappyhexmesh.h"
#include "base/boost_include.h"
#include "openfoam/openfoamcase.h"


// namespace insight
// {
//   
// namespace cfmeshFeats
// {
// 
// class Feature
// {
// public:
//   virtual void addIntoDictionary(OFDictData::dict& sHMDict) const =0;
//   virtual void modifyFiles(const OpenFOAMCase& ofc, 
// 		  const boost::filesystem::path& location) const;
//   virtual Feature* clone() const =0;
// };
// 
// inline Feature* new_clone(const Feature& op)
// {
//   return op.clone();
// }
// 
//   
// class Geometry
// : public ExternalGeometryFile
// {
//   CPPX_DEFINE_OPTIONCLASS(Parameters, ExternalGeometryFile::Parameters,
//       ( name, std::string, "" )
//       ( minLevel, int, 0 )
//       ( maxLevel, int, 4 )
//       ( nLayers, int, 2 )
//       ( zoneName, std::string, "" )
//   )
// 
// protected:
//   Parameters p_;
//   
// public:
//   Geometry(Parameters const& p = Parameters() );
//   
//   virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
//   virtual void modifyFiles(const OpenFOAMCase& ofc, 
// 		  const boost::filesystem::path& location) const;
//   
//   Feature* clone() const;
// };
// 
// class PatchLayers
// : public snappyHexMeshFeats::Feature
// {
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//       ( name, std::string, "" )
//       ( nLayers, int, 2 )
//   )
// 
// protected:
//   Parameters p_;
// 
// public:
//   PatchLayers(Parameters const& p = Parameters() );
//   
//   virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
//   
//   Feature* clone() const;
// };
// 
// 
// }
// 
// 
// namespace cfmeshOpts
// {
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (tlayer, double, 0.5)
//     (erlayer, double, 1.3)
//     (relativeSizes, bool, true)
//     (nLayerIter, int, 10 )
//     (stopOnBadPrismLayer, bool, false)
//   )
// };
// 
// void cfMesh
// (
//   const OpenFOAMCase& ofc, 
//   const boost::filesystem::path& location, 
//  
//   const boost::ptr_vector<cfmeshFeats::Geometry>& geo,
//  
//   cfmeshOpts::Parameters const& p = cfmeshOpts::Parameters(),
//   bool overwrite=true
// );
// 
// }


#endif
