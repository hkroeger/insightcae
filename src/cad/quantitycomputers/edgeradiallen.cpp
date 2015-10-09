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

#include "edgeradiallen.h"
#include "solidmodel.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{

edgeRadialLen::edgeRadialLen(const boost::shared_ptr<matQuantityComputer >& ax, const boost::shared_ptr<matQuantityComputer >& p0)
: ax_(ax), p0_(p0)
{
}

edgeRadialLen::~edgeRadialLen()
{
}

double edgeRadialLen::evaluate(FeatureID ei)
{
  arma::mat ax=ax_->evaluate(ei);
  arma::mat p0=p0_->evaluate(ei);
  
  arma::mat p1=(Vector(BRep_Tool::Pnt(TopExp::FirstVertex(model_->edge(ei))))).t()-p0;
  arma::mat p2=(Vector(BRep_Tool::Pnt(TopExp::LastVertex(model_->edge(ei))))).t()-p0;
  
  p1-=ax*(dot(ax, p1));
  p2-=ax*(dot(ax, p2));
  
  double rl=fabs(norm(p1,2)-norm(p2,2));
  std::cout<<"edge "<<ei<<": rl="<<rl<<std::endl;
  return rl;
}

QuantityComputer< double >::Ptr edgeRadialLen::clone() const
{
  return QuantityComputer<double>::Ptr(new edgeRadialLen(ax_, p0_));
}

}
}