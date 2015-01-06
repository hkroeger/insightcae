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


#include "stltools.h"

#include "boost/foreach.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{

  
  
arma::mat STLExtruder::tri::normal() const
{
  arma::mat d1=p[2]-p[0], d2=p[1]-p[0];
  arma::mat n=cross(d1, d2);
  n/=norm(n,2);
  return n;
}




void STLExtruder::addTriPair(const arma::mat& p0, const arma::mat& p1)
{
  tri t;
  t.p[0]=vec3(p0[0], p0[1], z0_);
  t.p[1]=vec3(p1[0], p1[1], z0_);
  t.p[2]=vec3(p0[0], p0[1], z1_);
  tris_.push_back(t);
  t.p[0]=vec3(p1[0], p1[1], z0_);
  t.p[1]=vec3(p1[0], p1[1], z1_);
  t.p[2]=vec3(p0[0], p0[1], z1_);
  tris_.push_back(t);
}




void STLExtruder::writeTris(const boost::filesystem::path& outputfilepath)
{
  std::ofstream f(outputfilepath.c_str());
  std::string name="patch0"; //outputfilepath.stem().string();
  f<<"solid "<<name<<std::endl;
  
  BOOST_FOREACH(const tri& t, tris_)
  {
    arma::mat n=t.normal();
    f<<"facet normal "<<n[0]<<" "<<n[1]<<" "<<n[2]<<endl;
    f<<" outer loop"<<endl;
    for (int j=0; j<3; j++)
      f<<"  vertex "<<t.p[j][0]<<" "<<t.p[j][1]<<" "<<t.p[j][2]<<endl;
    f<<" endloop"<<endl;
    f<<"endfacet"<<endl;
  }
  
  f<<"endsolid "<<name<<std::endl;
}




STLExtruder::STLExtruder
(
  const arma::mat xy_contour,
  double z0, double z1,
  const boost::filesystem::path& outputfilepath
)
: z0_(z0),
  z1_(z1)
{
  addTriPair(xy_contour.row(xy_contour.n_rows-1), xy_contour.row(0));
  for (int i=1; i<xy_contour.n_rows; i++)
  {    
    addTriPair(xy_contour.row(i-1), xy_contour.row(i));
  }
  
  writeTris(outputfilepath);
}
  
  
}
