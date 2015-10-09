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

#ifndef INSIGHT_CAD_APPROXIMATELYEQUAL_H
#define INSIGHT_CAD_APPROXIMATELYEQUAL_H

#include "feature.h"

namespace insight {
namespace cad {


template <class T>
class approximatelyEqual
    : public Filter
{
protected:
    boost::shared_ptr<QuantityComputer<T> > qtc1_;
    boost::shared_ptr<QuantityComputer<T> > qtc2_;
    double tol_;

public:
    approximatelyEqual(const QuantityComputer<T>& qtc1, const QuantityComputer<T>& qtc2, double tol)
        : qtc1_(qtc1.clone()),
          qtc2_(qtc2.clone()),
          tol_(tol)
    {}

    virtual void initialize(const SolidModel& m)
    {
        Filter::initialize(m);
        qtc1_->initialize(m);
        qtc2_->initialize(m);
    }

    virtual bool checkMatch(FeatureID f) const
    {
        T value1 = qtc1_->evaluate(f);
        T value2 = qtc2_->evaluate(f);

        T atol=tol_*value2;
        bool result = ( fabs(value1-value2) < atol );
        return result;
    }

    virtual FilterPtr clone() const
    {
        return FilterPtr(new approximatelyEqual(*qtc1_, *qtc2_, tol_));
    }
};


}
}

#endif // INSIGHT_CAD_APPROXIMATELYEQUAL_H
