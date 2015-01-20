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

#ifndef INSIGHT_AIRFOILSECTION_H
#define INSIGHT_AIRFOILSECTION_H

#include "openfoam/openfoamanalysis.h"
#include "base/stltools.h"

#include "base/boost_include.h"

namespace insight 
{


class AirfoilSection 
: public insight::OpenFOAMAnalysis
{
  std::string in_, out_, up_, down_, fb_, foil_;
public:
  declareType("Airfoil 2D");
  
  AirfoilSection(const NoParameters&);
    
  virtual insight::ParameterSet defaultParameters() const;

  virtual void createCase(insight::OpenFOAMCase& cm);
  virtual void createMesh(insight::OpenFOAMCase& cm);
  virtual insight::ResultSetPtr evaluateResults(insight::OpenFOAMCase& cm);
  
};

class AirfoilSectionPolar 
: public OpenFOAMParameterStudy
{
public:
    declareType("Airfoil 2D Polar");
    
    AirfoilSectionPolar(const NoParameters&);    
    virtual void evaluateCombinedResults(ResultSetPtr& results);
};

}

#endif // INSIGHT_AIRFOILSECTION_H
