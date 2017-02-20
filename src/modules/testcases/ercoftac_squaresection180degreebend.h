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

#ifndef INSIGHT_ERCOFTAC_SQUARESECTION180DEGREEBEND_H
#define INSIGHT_ERCOFTAC_SQUARESECTION180DEGREEBEND_H

#include "openfoam/openfoamanalysis.h"

namespace insight {

class ERCOFTAC_SquareSection180DegreeBend 
: public insight::OpenFOAMAnalysis
{
  double LinByD_, D_, HeByD_, nu_, Wb_;
  double grady_;
  int naxi_, naxo_, nbend_;
  
  /**
   * patch names
   */
  std::string in_, out_;
  
public:
  declareType("ERCOFTAC Square Section 180 Degree Bend");
  
    ERCOFTAC_SquareSection180DegreeBend(const ParameterSet& ps, const boost::filesystem::path& exepath);
    static insight::ParameterSet defaultParameters();
    virtual void calcDerivedInputData();
    virtual void createMesh(insight::OpenFOAMCase& cm);
    virtual void createCase(insight::OpenFOAMCase& cm);
    virtual insight::ResultSetPtr evaluateResults(insight::OpenFOAMCase& cm);
};
}

#endif // INSIGHT_ERCOFTAC_SQUARESECTION180DEGREEBEND_H
