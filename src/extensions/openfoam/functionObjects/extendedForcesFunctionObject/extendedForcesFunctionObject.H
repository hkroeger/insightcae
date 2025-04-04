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

#include "forcesources.h"

#include <boost/optional.hpp>

namespace Foam
{




class extendedForces
: public 
#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
 functionObjects::
#endif
   forces,
  public forceSource
{
protected:
  
  volVectorField* pressureForce_;
  volVectorField* viscousForce_;
  
  class Mask
  {
  public:
    word maskFieldName_;
    boost::optional<scalar> maskThreshold_;
    autoPtr<OFstream> maskedForceFile_, maskedForceFile2_;

    tmp<volScalarField> operator()(const fvMesh& mesh) const;

    Mask(const word& name, const dictionary& dict);

  };

  boost::optional<Mask> masking_;

  vector pr_force_;
  vector vi_force_;
  vector po_force_;
  vector pr_moment_;
  vector vi_moment_;
  vector po_moment_;
  
  void createFields();

#if OF_VERSION>=060500
  word phaseName_;
#endif

  tmp<volSymmTensorField> extendedDevRhoReff() const;

public:
  TypeName("extendedForces");
    // Constructors


        //- Construct for given objectRegistry and dictionary.
        //  Allow the possibility to load fields from files
#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
        extendedForces
        (
            const word& name,
            const Time&,
            const dictionary&
    #if OF_VERSION<060000 //not (defined(OFdev)||defined(OFesi1806))
            ,
            const bool readFields = true
    #endif
        );
#endif
        

        extendedForces
        (
            const word& name,
            const objectRegistry&,
            const dictionary&
#if OF_VERSION<040000 //not (defined(OFplus)||defined(OFdev)||defined(OFesi1806))
            ,
            const bool loadFromFiles = false
#endif
#if OF_VERSION>=010700 && OF_VERSION<=060000 //not (defined(OF16ext)||defined(OFdev)||defined(OFesi1806))
            ,
            const bool readFields = true
#endif
        );


#if OF_VERSION>=010700 && OF_VERSION<040000 //!(defined(OF16ext)||defined(OFplus)||defined(OFdev)||defined(OFesi1806))
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

#if OF_VERSION>=060500
    bool read(const dictionary&) override;
    void calcForcesMoment() override;
#endif

#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
    bool
#else
    void 
#endif
    execute() override;


#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
    bool
#else
    void 
#endif
    end() override;


#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
    bool
#else
    void 
#endif
    write() override;


    vector force() const override;
};

}

#endif
