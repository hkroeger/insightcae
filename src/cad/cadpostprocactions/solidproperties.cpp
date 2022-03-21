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
 */

#include "cadfeature.h"
#include "solidproperties.h"

#include "AIS_Point.hxx"
// #include "AIS_Drawer.hxx"
#include "Prs3d_TextAspect.hxx"
#include "occtools.h"

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(SolidProperties);

size_t SolidProperties::calcHash() const
{
  ParameterListHash h;
  h+=*model_;
  return h.getHash();
}


SolidProperties::SolidProperties(insight::cad::FeaturePtr model)
: model_(model)
{}

void SolidProperties::build()
{
  cout<<"######### SolidProperties Report ###########################################"<<endl;
  mass_=model_->mass();
  cout<<"mass="<<mass_<<endl;

  area_=model_->modelSurfaceArea();
  cout<<"area="<<area_<<endl;
  
  cog_=model_->modelCoG();
  cout<<"CoG = ["<<endl
    <<cog_
    <<" ]"<<endl;
  
  inertia_=model_->modelInertia();
  cout<<"inertia tensor = [ "<<endl
       <<inertia_
       <<" ]"<<endl;
  
  arma::mat bb=model_->modelBndBox(/*0.01*/);
  bb_pmin_=bb.col(0);
  bb_pmax_=bb.col(1);
}

void SolidProperties::write(ostream&) const
{

}


}
}
