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

#ifndef INSIGHT_CAD_BOOLEANFILTERS_H
#define INSIGHT_CAD_BOOLEANFILTERS_H

#include "featureset.h"

namespace insight {
namespace cad {




class AND
    : public Filter, public EnableCreateFunction<AND>
{
protected:
    FilterPtr f1_;
    FilterPtr f2_;
public:
    AND(FilterArg f1, FilterArg f2);
    virtual void initialize(ConstFeaturePtr m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};




class OR
    : public Filter, public EnableCreateFunction<OR>
{
protected:
    FilterPtr f1_;
    FilterPtr f2_;
public:
    OR(FilterArg f1, FilterArg f2);
    virtual void initialize(ConstFeaturePtr m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};




class NOT
    : public Filter, public EnableCreateFunction<NOT>
{
protected:
    FilterPtr f1_;
public:
    NOT(FilterArg f1);
    virtual void firstPass(FeatureID feature);
    virtual void initialize(ConstFeaturePtr m);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};


FilterPtr operator&&(FilterPtr& f1, FilterPtr f2);
FilterPtr operator!(FilterPtr f);


}
}

#endif // INSIGHT_CAD_BOOLEANFILTERS_H
