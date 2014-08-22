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


#ifndef INSIGHT_PLOTTOOLS_H
#define INSIGHT_PLOTTOOLS_H

#include "base/resultset.h"
#include "base/linearalgebra.h"
#include "boost/foreach.hpp"

#include <iostream>

namespace insight
{

void mplDataRead(std::ostream& f, const std::string& name, TabularResult::Table& data);
void mplDataRead(std::ostream& f, const std::string& name, const arma::mat& x, const arma::mat& y);

}

#endif // INSIGHT_PLOTTOOLS_H
