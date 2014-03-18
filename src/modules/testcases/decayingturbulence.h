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
protected:
  std::string inlet_, outlet_;
  
public:
    declareType("Decaying Turbulence Test Case");
    
    DecayingTurbulence(const NoParameters&);
    ~DecayingTurbulence();
    
    int calcnh(const ParameterSet& p) const;
    double calcT(const ParameterSet& p) const;    
    
    ParameterSet defaultParameters() const;
    
    virtual void createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p);
    virtual void createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p);
    
    virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
    virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
};

}

#endif // INSIGHT_DECAYINGTURBULENCE_H
