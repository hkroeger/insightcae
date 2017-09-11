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
: public 
#ifdef OFplus
 functionObjects::
#endif
 forces
     
{
protected:
  
  volVectorField* pressureForce_;
  volVectorField* viscousForce_;
  
  void createFields();
  
public:
  TypeName("extendedForces");
    // Constructors

        //- Construct for given objectRegistry and dictionary.
        //  Allow the possibility to load fields from files
#ifdef OFplus
        extendedForces
        (
            const word& name,
            const Time&,
            const dictionary&,
            const bool readFields = true
        );
#endif
        
        extendedForces
        (
            const word& name,
            const objectRegistry&,
            const dictionary&
#ifndef OFplus
            ,
            const bool loadFromFiles = false
#endif
#ifndef OF16ext
            ,
            const bool readFields = true
#endif
        );

#if !(defined(OF16ext)||defined(OFplus))
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
#endif

    //- Destructor
    virtual ~extendedForces();  

    virtual 
#ifdef OFplus
    bool
#else
    void 
#endif
    execute();
    
    virtual 
#ifdef OFplus
    bool
#else
    void 
#endif
    end();
  
};

}

#endif
