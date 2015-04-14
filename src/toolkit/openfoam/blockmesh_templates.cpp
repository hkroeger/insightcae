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

#include "base/boost_include.h"
#include "blockmesh_templates.h"

using namespace boost;
using namespace boost::assign;

namespace insight {
  
namespace bmd
{

Cylinder::Cylinder
(
  blockMesh& bmd, 
  const insight::ParameterSet& p, 
  insight::bmd::Patch* lowxendPatch, 
  insight::bmd::Patch* highxendPatch, 
  insight::bmd::Patch* outerPatch
)
: bmd_(bmd), p_(p)
{

  double al = M_PI/2.;
  
  double Lc=p_.geometry.D*0.33;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (2, 	vec3(0, 0.5*p_.geometry.D, 0))
      (1, 	vec3(0,  ::cos(al)*Lc, 0.))
      .convert_to_container<std::map<int, Point> >()
  ;
  arma::mat vL=p_.geometry.L*p_.geometry.ex;
  
  // core block
  {
    arma::mat r0=rotMatrix(0.5*al, p_.geometry.ex);
    arma::mat r1=rotMatrix(1.5*al, p_.geometry.ex);
    arma::mat r2=rotMatrix(2.5*al, p_.geometry.ex);
    arma::mat r3=rotMatrix(3.5*al, p_.geometry.ex);

    Block& bl = bmd.addBlock
    (  
      new Block(P_8(
	  r1*pts[0], r2*pts[0], r3*pts[0], r0*pts[0],
	  (r1*pts[0])+vL, (r2*pts[0])+vL, (r3*pts[0])+vL, (r0*pts[0])+vL
	),
	p_.mesh.nu, p_.mesh.nu, p_.mesh.nx
      )
    );
    if (lowxendPatch) lowxendPatch->addFace(bl.face("0321"));
    if (highxendPatch) highxendPatch->addFace(bl.face("4567"));
  }

  // radial blocks
  for (int i=0; i<4; i++)
  {
    arma::mat r0=rotMatrix(double(i+0.5)*al, p_.geometry.ex);
    arma::mat r1=rotMatrix(double(i+1.5)*al, p_.geometry.ex);

    {    
      Block& bl = bmd.addBlock
      (
	new Block(P_8(
	    r1*pts[0], r0*pts[0], r0*pts[1], r1*pts[1],
	    (r1*pts[0])+vL, (r0*pts[0])+vL, (r0*pts[1])+vL, (r1*pts[1])+vL
	  ),
	  p_.mesh.nu, p_.mesh.nr, p_.mesh.nx,
	  list_of<double>(1)(1./p_.mesh.gradr)(1)
	)
      );
      if (lowxendPatch) lowxendPatch->addFace(bl.face("0321"));
      if (highxendPatch) highxendPatch->addFace(bl.face("4567"));
    }


    arma::mat rmid=rotMatrix(double(i+1)*al, p_.geometry.ex);
    bmd.addEdge(new ArcEdge(r1*pts[1], r0*pts[1], rmid*pts[1]));
    bmd.addEdge(new ArcEdge((r1*pts[1])+vL, (r0*pts[1])+vL, (rmid*pts[1])+vL));

    //inner core
//     bmd->addEdge(new ArcEdge(r1*pts[0], r0*pts[0], rmid*pts[]));
//     bmd->addEdge(new ArcEdge((r1*pts[0])+vL, (r0*pts[0])+vL, (rmid*pts[])+vL));

  }

}

  
}

}