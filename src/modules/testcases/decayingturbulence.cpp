/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "decayingturbulence.h"
#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/basiccaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{
  
defineType(DecayingTurbulence);
addToFactoryTable(Analysis, DecayingTurbulence, NoParameters);

DecayingTurbulence::DecayingTurbulence(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Decaying Turbulence Test Case",
    "Turbulent flow entering domain and decaying inside"
  ),
  inlet_("inlet"),
  outlet_("outlet")
{}

DecayingTurbulence::~DecayingTurbulence()
{}

void DecayingTurbulence::createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{

}

void DecayingTurbulence::createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{

}

}