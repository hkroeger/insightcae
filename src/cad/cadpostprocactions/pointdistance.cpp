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
#include "pointdistance.h"

#include "AIS_Point.hxx"
// #include "AIS_Drawer.hxx"
#include "Prs3d_TextAspect.hxx"
#include "occtools.h"

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(Distance);

size_t Distance::calcHash() const
{
  ParameterListHash h;
  h+=p1_->value();
  h+=p2_->value();
  return h.getHash();
}


Distance::Distance(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

void insight::cad::Distance::build()
{
  arma::mat p1=p1_->value();
  arma::mat p2=p2_->value();

  cout<<"######### Distance Report ###########################################"<<endl;
  distance_=arma::norm(p2-p1,2);
  cout<<"distance="<<distance_<<endl;
}

void insight::cad::Distance::write(ostream&) const
{}



defineType(DistanceConstraint);

size_t DistanceConstraint::calcHash() const
{
    ParameterListHash h;
    h+=p1_->value();
    h+=p2_->value();
    h+=targetValue_->value();
    return h.getHash();
}


DistanceConstraint::DistanceConstraint(VectorPtr p1, VectorPtr p2, ScalarPtr targetValue)
    : Distance(p1, p2),
      targetValue_(targetValue)
{}

ScalarPtr DistanceConstraint::targetValue()
{
    return targetValue_;
}


int DistanceConstraint::nConstraints() const
{
    return 1;
}

double DistanceConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
                iConstraint==0,
                "invalid constraint id" );
    checkForBuildDuringAccess();
    return distance_ - targetValue_->value();
}



}
}
