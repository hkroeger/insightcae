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

#ifndef INSIGHT_CAD_RELATIONFILTERS_H
#define INSIGHT_CAD_RELATIONFILTERS_H

#include "feature.h"

namespace insight
{
namespace cad
{


#define RELATION_QTY_FILTER(RELATION_QTY_FILTER_NAME, RELATION_QTY_FILTER_OP) \
template <class T1, class T2>\
class RELATION_QTY_FILTER_NAME\
: public Filter\
{\
protected:\
  boost::shared_ptr<QuantityComputer<T1> > qtc1_;\
  boost::shared_ptr<QuantityComputer<T2> > qtc2_;\
  \
public:\
  RELATION_QTY_FILTER_NAME(const QuantityComputer<T1>& qtc1, const QuantityComputer<T2>& qtc2)\
  : qtc1_(qtc1.clone()),\
    qtc2_(qtc2.clone())\
  {}\
  \
  virtual void initialize(const SolidModel& m)\
  {\
    Filter::initialize(m);\
    qtc1_->initialize(m);\
    qtc2_->initialize(m);\
  }\
  virtual bool checkMatch(FeatureID f) const\
  {\
    T1 value1 = qtc1_->evaluate(f);\
    T2 value2 = qtc2_->evaluate(f);\
    bool result = (RELATION_QTY_FILTER_OP);\
    return result;\
  }\
  \
  virtual FilterPtr clone() const\
  {\
    return FilterPtr(new RELATION_QTY_FILTER_NAME(*qtc1_, *qtc2_));\
  }\
};

RELATION_QTY_FILTER(greater, (value1>value2));
RELATION_QTY_FILTER(greaterequal, (value1>=value2));
RELATION_QTY_FILTER(less, (value1<value2));
RELATION_QTY_FILTER(lessequal, (value1<=value2));
RELATION_QTY_FILTER(equal, (value1==value2));


}
}

#endif