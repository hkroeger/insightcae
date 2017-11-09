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

#ifndef INSIGHT_DECAYINGTURBULENCE_H
#define INSIGHT_DECAYINGTURBULENCE_H

#include "openfoam/openfoamanalysis.h"

namespace insight {

class DecayingTurbulence 
: public insight::OpenFOAMAnalysis
{
public:
#include "decayingturbulence__DecayingTurbulence__Parameters.h"
/*
PARAMETERSET>>> DecayingTurbulence Parameters
inherits OpenFOAMAnalysis::Parameters

geometry = set {
 H = double 1.0 "[m] Height and width of the domain"
 L = double 2.0 "[m] Length of the domain"
} "Geometrical properties of the domain"
      
mesh = set {
 nax = int 100 "# cells in axial direction"
 s = double 1.0 "Axial grid anisotropy (ratio of axial cell edge length to lateral edge length)"
} "Properties of the computational mesh"

fluid = set 
{
  nu = double 1.8e-5 "[m^2/s] Viscosity of the fluid"
} "Parameters of the fluid"
   
operation = set {
 U = double 1 "[m/s] Inflow velocity"
} "Definition of the operation point under consideration"
     
<<<PARAMETERSET
*/

protected:
  std::string inlet_, outlet_;
  
public:
    declareType("Decaying Turbulence Test Case");
    
    DecayingTurbulence(const ParameterSet& ps, const boost::filesystem::path& exepath);
    ~DecayingTurbulence();
    
    int calcnh() const;
    double calcT() const;    
    
    static ParameterSet defaultParameters();
    static std::string category() { return "Validation Cases/Inflow Generator"; }
    
    virtual void createCase(insight::OpenFOAMCase& cm);
    virtual void createMesh(insight::OpenFOAMCase& cm);
    
    virtual void applyCustomPreprocessing(OpenFOAMCase& cm);
    virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
};

}

#endif // INSIGHT_DECAYINGTURBULENCE_H
