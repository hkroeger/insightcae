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

#include <memory>
#include <set>
#include <vector>


namespace Foam
{
  



class forceSource
{
protected:
    word lbl_;
    bool registered_;

public:
    forceSource(const word& lbl, bool autoRegister=true);
    virtual ~forceSource();

    const word& faceSourceLabel() const;
    virtual vector force() const =0;
};




class constantForceSource
        : public forceSource
{
protected:
    vector F_;

public:
    constantForceSource(const word& lbl, const vector& F, bool autoRegister=true);

    vector force() const override;
};




class subtractedForceSource
        : public forceSource
{
protected:
    forceSource *a_, *b_;

public:
    subtractedForceSource(const word& lbl, forceSource* a, forceSource* b, bool autoRegister=true);

    vector force() const override;
};




class multipliedForceSource
        : public forceSource
{
protected:
    scalar multiplier_;
    forceSource *f_;

public:
    multipliedForceSource(const word& lbl, scalar m, forceSource* f, bool autoRegister=true);

    vector force() const override;
};



class forceSources
        : private std::set<forceSource*>
{

    forceSources();

public:
    void registerSource(forceSource* fs);
    void unregisterSource(forceSource* fs);
    bool exists(const word& lbl) const;
    forceSource* get(const word& lbl) const;

    static forceSources& registry();
};




class forceSourceCombination
{
    std::string definition_;
    std::vector<std::shared_ptr<forceSource> > intermediateSources_;
    forceSource* value_;

    void interpretDefinition();
    forceSource* parseSource(Istream& is);

public:
    forceSourceCombination();
    forceSourceCombination(ITstream& is);
    forceSourceCombination(forceSource* value);
    vector force() const;
};




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
  
  word maskFieldName_;
  autoPtr<OFstream> maskedForceFile_, maskedForceFile2_;
  vector pr_force_;
  vector vi_force_;
  vector po_force_;
  vector pr_moment_;
  vector vi_moment_;
  vector po_moment_;
  
  void createFields();
  
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

    //- Destructor
    virtual ~extendedForces();  

    virtual 
#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
    bool
#else
    void 
#endif
    execute();
    
    virtual 
#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
    bool
#else
    void 
#endif
    end();

    virtual 
#if OF_VERSION>=040000 //(defined(OFplus)||defined(OFdev)||defined(OFesi1806))
    bool
#else
    void 
#endif
    write();

    vector force() const override;
};

}

#endif
