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

#include "freeship_interface.h"

namespace insight {
namespace cad {
  
void writeFreeShipSurface(const SolidModel& hull, const boost::filesystem::path& filepath)
{
  std::ofstream f(filepath.c_str());
  f<<"0"<<endl;
  
  arma::mat bb=hull.modelBndBox();
  double L=bb(0,1)-bb(0,0);
  
  int nL=20;
  int np=100;
  
  for (int i=0; i<nL; i++)
  {
    double x=std::max(1e-3, std::min(0.999, double(i)/double(nL-1)));
    gp_Pln pln( gp_Pnt(x*L+bb(0,0), 0, 0), gp_Dir(1,0,0) );
    
    TopoDS_Face isec=TopoDS::Face(BRepAlgoAPI_Section(hull, pln));
    TopoDS_Wire ow=BRepTools::OuterWire(isec);
    BRepAdaptor_CompCurve cc(ow);
    double t0=cc.FirstParameter(), t1=cc.LastParameter();
    
    for (int j=0; j<np; j++)
    {
      double t=t0+(t1-t0)*double(j)/double(np-1);
      gp_Pnt p=cc.Value(t);
      if (p.Y()>=0.0)
	f<<p.X()<<" "<<p.Y()<<" "<<p.Z()<<endl;
    }
    
    f<<endl;
  }
}

void writeHydrostaticReport
(
  const SolidModel& hullvolume,
  const SolidModel& shipmodel,
  const arma::mat& psurf, const arma::mat& nsurf, 
  const boost::filesystem::path& outputfilepath
)
{
  cout<<"writing to "<<outputfilepath<<endl;
  std::ofstream f(outputfilepath.c_str());
  
  Cutaway submergedvolume(hullvolume, psurf, nsurf);
  
  double V=submergedvolume.modelVolume();
  f<<"# Volume of displacement"<<endl;
  f<<"V = "<<V<<endl;

  arma::mat G=shipmodel.modelCoG();
  f<<"# Centre of gravity"<<endl;
  f<<"G = ["<<G(0)<<", "<<G(1)<<", "<<G(2)<<"]"<<endl;
  
  arma::mat B=submergedvolume.modelCoG();
  f<<"# Centre of buoyancy"<<endl;
  f<<"B = ["<<B(0)<<", "<<B(1)<<", "<<B(2)<<"]"<<endl;
  
  submergedvolume.saveAs("test.stp");
}


}
}