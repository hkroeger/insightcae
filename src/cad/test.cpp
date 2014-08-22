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

#include "boost/foreach.hpp"
#include "base/linearalgebra.h"
#include "solidmodel.h"
#include "sketch.h"
#include "parser.h"
#include "boost/exception/diagnostic_information.hpp"

using namespace boost;
using namespace insight;
using namespace insight::cad;

int main(int argc, char* argv[])
{
  /*
  arma::mat p0=vec3(0,0,0);
  arma::mat ax=vec3(0,0,1), ax2=vec3(0,1,0);

  Box b(p0-4.5*ax, vec3(0,0.2,0.7), vec3(0,-0.3,0.7), vec3(0.7,0.2,0));
  
  SolidModel m=
    (Cylinder(p0-5.*ax, p0+5.*ax, 1) 
    | 
    Sphere(p0+5.*ax, 1)
    |
    Cylinder(p0-5.0*ax2, p0+5.*ax2, 1.1)
    )-
    b
    ;
    
  FeatureSet e;
  
  e = m.query_edges( coincident<Edge>(b) );
  cout<<e<<endl;
  
  SolidModel m2=Fillet(m, e, 0.05);
  
  
  DatumPlane p(vec3(0,5,0), vec3(0.2,0.2,1));
  Extrusion m3(Sketch(p, "sketch1.dxf"), vec3(0,0,10));
  
  m3.saveAs( "shape.brep" );
  */
  try{
  parser::Model::Ptr m;
  bool ok=parseISCADModelFile(argv[1], m);
  cout<<"OK="<<ok<<endl;
//   BOOST_FOREACH(const parser::modelstep& ms, m->modelstepSymbols)
//   {
//     cout<<ms.first<<endl;
//   }
  }
  catch (boost::exception& ex) {
    // error handling
    std::cerr << boost::diagnostic_information(ex);
  }
//   
//   e = m.query_edges( edgeTopology(GeomAbs_Circle) );
//   cout<<e<<endl;
//   
//   e = m.query_edges
//   ( 
//     edgeTopology(GeomAbs_Circle) 
//     && 
//     ( as_scalar<arma::mat>( arma::mat(ax.t()) * edgeCoG() ) > 0.0) 
//   );
//   cout<<e<<endl;
//   
//   e = m.query_edges( !edgeTopology(GeomAbs_Circle) && !edgeTopology(GeomAbs_Line) );
//   cout<<e<<endl;
  
  return 0;
}