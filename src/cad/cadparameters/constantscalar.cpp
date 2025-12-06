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

#include "constantscalar.h"
#include "cadfeature.h"
#include "datum.h"


void insight::cad::ConstantScalar::replaceDependency(const DependencyReplacement& repl)
{}

void  insight::cad::ConstantScalar::addDependencies(DependencyList& dl) const
{}


insight::cad::ConstantScalar::ConstantScalar(double value)
: value_(value)
{}

size_t insight::cad::ConstantScalar::calcHash() const
{
    ParameterListHash h;
    h+=value_;
    return h.getHash();
}

double insight::cad::ConstantScalar::calcValue() const
{
    return value_;
}


std::shared_ptr<insight::cad::DependencySource>
insight::cad::ConstantScalar::shallowClone(TreeCloneMap &tcm) const
{
    return std::shared_ptr<insight::cad::DependencySource>(
        new ConstantScalar(value_)
        );
}

insight::cad::ScalarPtr insight::cad::scalarconst(double v)
{
  return ScalarPtr(new ConstantScalar(v));
}
