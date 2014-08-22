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

#ifndef INSIGHT_EXTENDEDFORCEFUNCTIONOBJECT_H
#define INSIGHT_EXTENDEDFORCEFUNCTIONOBJECT_H

#include "forces.H" 

namespace Foam
{
  
class extendedForces
: public forces
{
protected:
  
  autoPtr<volVectorField> pressureForce_, viscousForce_;
  
public:
  TypeName("extendedForces");
    // Constructors

        //- Construct for given objectRegistry and dictionary.
        //  Allow the possibility to load fields from files
        extendedForces
        (
            const word& name,
            const objectRegistry&,
            const dictionary&,
            const bool loadFromFiles = false,
            const bool readFields = true
        );


        //- Construct from components
        extendedForces
        (
            const word& name,
            const objectRegistry&,
            const labelHashSet& patchSet,
            const word& pName,
            const word& UName,
            const word& rhoName,
            const scalar rhoInf,
            const scalar pRef,
            const coordinateSystem& coordSys
        );


    //- Destructor
    virtual ~extendedForces();  

    virtual void execute();
  
};

}

#endif