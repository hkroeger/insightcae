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
#ifndef INSIGHT_INTERNALPRESSURELOSS_H
#define INSIGHT_INTERNALPRESSURELOSS_H

#include <openfoam/openfoamanalysis.h>
#include "cadtypes.h"

namespace insight
{
  
class InternalPressureLoss
: public insight::OpenFOAMAnalysis
{
public:
  
#include "internalpressureloss__InternalPressureLoss__Parameters.h"
/*
PARAMETERSET>>> InternalPressureLoss Parameters

geometry=selectablesubset {{

 STEP set {
      cadmodel          = path 		"cadmodel.stp" 		"CAD model file."
      inlet_name	= string	"INLET"			"Name of inlet surface in CAD model."
      outlet_name	= string	"OUTLET"		"Name of outlet surface in CAD model."
 }

 STL set {
      cadmodel          = path 		"cadmodel.stlb" 	"Triangulated geometry, excluding in- and outlet."
      inlet             = path          "inlet.stlb"		"Triangulated geometry of inlet alone."
      outlet    	= path          "outlet.stlb"		"Triangulated geometry of outlet alone."
 }

}} STEP "Specification of geometry"

mesh=set
{
  size		= double	10 		"[mm] Cell size of template mesh."
  minLevel      = int           0               "Minimum refinement level on geometry."
  maxLevel      = int           2               "Maximum refinement level on geometry."
} "Properties of the computational mesh"

operation=set
{
  Q		= double 	0.1 		"[m^3/s] volumetric flux into inlet"
} "Definition of the operation point under consideration"

fluid=set
{
  rho		= double 	998.0 		"[kg/m^3] Density of the fluid"
  nu		= double 	1e-6 		"[m^2/s] Viscosity of the fluid"
} "Parameters of the fluid"

<<<PARAMETERSET
*/

protected:
    // derived data
    insight::cad::FeaturePtr walls_, inlet_, outlet_;
    arma::mat bb_, bbi_, L_;
    
    /**
     * hydraulic diameter of inlet
     */
    double D_;
    
    /**
     * area of inlet boundary
     */
    double Ain_;
    
    int nx_, ny_, nz_;
    
    boost::filesystem::path wallstlfile_, inletstlfile_, outletstlfile_;
  
public:
    declareType("InternalPressureLoss");
    InternalPressureLoss(const ParameterSet& ps, const boost::filesystem::path& exepath);
    static ParameterSet defaultParameters();
    static std::string category() { return "Generic Analyses"; }
    
    virtual void calcDerivedInputData();
    virtual void createCase(insight::OpenFOAMCase& cm);
    virtual void createMesh(insight::OpenFOAMCase& cm);
    
    virtual ResultSetPtr evaluateResults(OpenFOAMCase& cmp);
};

}

#endif // INSIGHT_INTERNALPRESSURELOSS_H
