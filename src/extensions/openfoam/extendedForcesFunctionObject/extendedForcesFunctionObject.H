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