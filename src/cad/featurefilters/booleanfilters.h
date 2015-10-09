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

#include "feature.h"

namespace insight {
namespace cad {


class AND
    : public Filter
{
protected:
    FilterPtr f1_;
    FilterPtr f2_;
public:
    AND(const Filter& f1, const Filter& f2);
    virtual void initialize(const SolidModel& m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class OR
    : public Filter
{
protected:
    FilterPtr f1_;
    FilterPtr f2_;
public:
    OR(const Filter& f1, const Filter& f2);
    virtual void initialize(const SolidModel& m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class NOT
    : public Filter
{
protected:
    FilterPtr f1_;
public:
    NOT(const Filter& f1);
    virtual void firstPass(FeatureID feature);
    virtual void initialize(const SolidModel& m);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};


}
}

#endif // INSIGHT_CAD_BOOLEANFILTERS_H
