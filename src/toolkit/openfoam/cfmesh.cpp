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

#include "cfmesh.h"

// namespace insight
// {
// 
//   
// namespace cfmeshFeats 
// {
// 
// void Feature::modifyFiles(const OpenFOAMCase& ofc, const boost::filesystem::path& location) const
// {
// }
// 
// 
// Geometry::Geometry(const Geometry::Parameters& p)
// : ExternalGeometryFile(p),
//   p_(p)
// {}
// 
// void Geometry::addIntoDictionary(OFDictData::dict& sHMDict) const
// {
//   OFDictData::dict geodict;
//   geodict["type"]="triSurfaceMesh";
//   geodict["name"]=p_.name();
//   //boost::filesystem::path x; x.f
//   sHMDict.subDict("geometry")[p_.fileName().filename().c_str()]=geodict;
// 
//   OFDictData::dict castdict;
//   OFDictData::list levels;
//   levels.push_back(p_.minLevel());
//   levels.push_back(p_.maxLevel());
//   castdict["level"]=levels;
//   if (p_.zoneName()!="")
//   {
//     castdict["faceZone"]=p_.zoneName();
//     castdict["cellZone"]=p_.zoneName();
//     castdict["cellZoneInside"]="inside";
//   }
//   sHMDict.subDict("castellatedMeshControls").subDict("refinementSurfaces")[p_.name()]=castdict;
// 
//   OFDictData::dict layerdict;
//   layerdict["nSurfaceLayers"]=p_.nLayers();
//   sHMDict.subDict("addLayersControls").subDict("layers")["\""+p_.name()+".*\""]=layerdict;
// 
// }
// 
// 
// void Geometry::modifyFiles(const OpenFOAMCase& ofc, const boost::filesystem::path& location) const
// {
// //     Feature::modifyFiles(ofc, location);
//     ExternalGeometryFile::putIntoConstantTrisurface(ofc, location);
// }
// 
// 
// 
// }
//   
// 
// void cfMesh
// (
//   const OpenFOAMCase& ofc, 
//   const boost::filesystem::path& location,
//   const OFDictData::list& PiM,
//   const boost::ptr_vector<cfmeshFeats::Geometry>& geo,
//   snappyHexMeshOpts::Parameters const& p,
//   bool overwrite
// )
// {/*
//   using namespace cfmeshFeats;
//   
//   // write domain geometry surface
//   
//   // combine into single surface file
//   
//   // surfaceFeatureEdges
//   
//   OFDictData::dictFile meshDict;
//   
//   // setup dict structure
// 
//   // insert geometry refinement
//   BOOST_FOREACH( const snappyHexMeshFeats::Feature& feat, ops)
//   {
//     feat.modifyFiles(ofc, location);
//     feat.addIntoDictionary(meshDict);
//   }
// 
//   // insert remaining features
//   BOOST_FOREACH( const snappyHexMeshFeats::Feature& feat, ops)
//   {
//     feat.modifyFiles(ofc, location);
//     feat.addIntoDictionary(meshDict);
//   }
//   
//   // then write to file
//   boost::filesystem::path dictpath = location / "system" / "meshDict";
//   if (!exists(dictpath.parent_path())) 
//   {
//     boost::filesystem::create_directories(dictpath.parent_path());
//   }
//   
//   {
//     std::ofstream f(dictpath.c_str());
//     writeOpenFOAMDict(f, meshDict, boost::filesystem::basename(dictpath));
//   }
// 
//   std::vector<std::string> opts;
// //   if (overwrite) opts.push_back("-overwrite");
//         
//   int np=readDecomposeParDict(location);
//   bool is_parallel = (np>1);
// 
//   if (is_parallel)
//   {
//     ofc.executeCommand(location, "decomposePar");
//   }
//   
//   //cm.runSolver(executionPath(), analyzer, solverName, &stopFlag_, np);
//   std::vector<std::string> output;
//   ofc.executeCommand(location, "cartesianMesh", opts, &output, np);
// 
//   
// //   // Check fraction of extruded faces on wall patches
// //   boost::regex re_extrudedfaces("^Extruding ([0-9]+) out of ([0-9]+) faces.*");
// //   boost::match_results<std::string::const_iterator> what;
// //   int exfaces=-1, totalfaces=-1;
// //   BOOST_FOREACH(const std::string& line, output)
// //   {
// //     if (boost::regex_match(line, what, re_extrudedfaces))
// //     {
// //       //cout<< "\""<<line<<"\""<<what[1]<<", "<<what[2]<<endl;
// //       exfaces=lexical_cast<int>(what[1]);
// //       totalfaces=lexical_cast<int>(what[2]);
// //     }
// //   }
// //   if (totalfaces>=0)
// //   {
// //     double exfrac=double(exfaces)/double(totalfaces);
// //     if (exfrac<0.9)
// //     {
// //       std::string msg=
// //       "Prism layer covering is only "+str(format("%g")%(100.*exfrac))+"\% (<90%)!\n"
// //       "Please reconsider prism layer thickness and tune number of prism layers!";
// //       
// //       if (p.stopOnBadPrismLayer())
// //       {
// // 	throw insight::Exception(msg);
// //       }
// //       else
// // 	insight::Warning(msg);
// //     }
// //   }
//   
//   if (is_parallel)
//   {
//     ofc.executeCommand(location, "reconstructParMesh", list_of<string>("-constant") );
//     ofc.removeProcessorDirectories(location);
//   }
//   
//   
//   //ofc.executeCommand(location, "snappyHexMesh", opts);
//   */
// }
// 
// }
