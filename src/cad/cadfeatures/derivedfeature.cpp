 
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

#include "derivedfeature.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
  
defineType(DerivedFeature);


DerivedFeature::DerivedFeature(const NoParameters& nop)
: Feature(nop)
{}

DerivedFeature::DerivedFeature(ConstFeaturePtr basefeat)
: basefeat_(basefeat)
{

}

double DerivedFeature::density() const 
{ 
  if (density_)
    return density_->value(); 
  else
    return basefeat_->density();
}


double DerivedFeature::areaWeight() const 
{ 
  if (areaWeight_)
    return areaWeight_->value(); 
  else
    return basefeat_->areaWeight();
}


double DerivedFeature::mass(double density_ovr, double aw_ovr) const
{
  double rho=density_ovr, aw=aw_ovr;
  if (density_ && (density_ovr<0.)) rho=density_->value();
  if (areaWeight_ && (aw_ovr<0.)) aw=areaWeight_->value();

  double m;
  if (isTransformationFeature())
  {
    m=basefeat_->mass(rho, aw);
  }
  else
  {
    m=Feature::mass(rho, aw);
  }
//   std::cout<<"DerivedFeature: "<<rho<<" m="<<m<<" reloc:"<<isTransformationFeature()<<std::endl;
  return m;
}

arma::mat DerivedFeature::modelCoG(double density_ovr) const
{
  double rho=density_ovr/*, aw=aw_ovr*/;
  if (density_ && (density_ovr<0.)) rho=density_->value();
//   if (areaWeight_ && (aw_ovr<0.)) aw=areaWeight_->value();

  if (isTransformationFeature())
    return rotTrsf(transformation())*basefeat_->modelCoG(rho) + transTrsf(transformation());
  else
    return Feature::modelCoG(rho);
}

arma::mat DerivedFeature::modelInertia(double density_ovr) const
{
  double rho=density_ovr/*, aw=aw_ovr*/;
  if (density_ && (density_ovr<0.)) rho=density_->value();
//   if (areaWeight_ && (aw_ovr<0.)) aw=areaWeight_->value();

  if (isTransformationFeature())
  {
    arma::mat d=transTrsf(transformation());
    arma::mat dt=arma::zeros(3,3);
    dt(0,1)=-d(2);
    dt(0,2)=d(1);
    dt(1,2)=-d(0);
    dt(1,0)=d(2);
    dt(2,0)=-d(1);
    dt(2,1)=d(0);
    return basefeat_->modelInertia(rho) + basefeat_->mass(rho)*dt.t()*dt;
  }
  else
    return Feature::modelInertia(rho);
}

bool DerivedFeature::isSingleEdge() const
{
    checkForBuildDuringAccess();
    return basefeat_->isSingleEdge();
}

bool DerivedFeature::isSingleOpenWire() const
{
    checkForBuildDuringAccess();
    return basefeat_->isSingleOpenWire();
}

bool DerivedFeature::isSingleClosedWire() const
{
    checkForBuildDuringAccess();
    return basefeat_->isSingleClosedWire();
}

bool DerivedFeature::isSingleWire() const
{
    checkForBuildDuringAccess();
    return basefeat_->isSingleWire();
}

bool DerivedFeature::isSingleFace() const
{
    checkForBuildDuringAccess();
    return basefeat_->isSingleFace();
}

bool DerivedFeature::isSingleVolume() const
{
    checkForBuildDuringAccess();
    return basefeat_->isSingleVolume();
}


}
}
