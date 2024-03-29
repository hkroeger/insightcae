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

#ifndef faceQualityMarkerFunctionObject_H
#define faceQualityMarkerFunctionObject_H

#include "fvCFD.H"
#include "functionObject.H"
#include "dictionary.H"
#include "Switch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class faceQualityMarkerFunctionObject Declaration
\*---------------------------------------------------------------------------*/
class faceSet;

class faceQualityMarkerFunctionObject
: public functionObject
{
  
    // Private data
    //- Disallow default bitwise copy construct
    faceQualityMarkerFunctionObject(const faceQualityMarkerFunctionObject&);

    //- Disallow default bitwise assignment
    void operator=(const faceQualityMarkerFunctionObject&);
  
protected:

    //- Reference to main object registry
    const Time& time_;

    //- Region name
    wordList regionNames_;
    List<List<surfaceScalarField*> > blendingFactors_;

    bool markNonOrthFaces_;
    bool markSkewFaces_;
    bool markWarpedFaces_;
    bool markConcaveFaces_;
    bool markHighAspectFaces_;
    bool markLowQualityTetFaces_;
    bool markVelocityPeaks_;
    bool smoothMarkerField_;
    bool updateOnMeshChange_;
    scalar aspectThreshold_;
    scalar lowerNonOrthThreshold_;
    scalar upperNonOrthThreshold_;
    scalar smoothingCoeff_;
    wordList sets_;

    List<const polyMesh*> meshes_;
    wordList blendingFieldNames_;

// Private Member Functions


    void markFaceSet(label mi, const faceSet& faces);
    void updateBlendingFactor();

public:

    //- Runtime type information
    TypeName("faceQualityMarker");


    // Constructors

        //- Construct from components
        faceQualityMarkerFunctionObject
        (
            const word& name,
            const Time&,
            const dictionary&
        );
	
	virtual ~faceQualityMarkerFunctionObject();


    // Member Functions

        //- start is called at the start of the time-loop
#if OF_VERSION<040000 //not (defined(OFdev)||defined(OFplus)||defined(OFesi1806))
        virtual 
#endif
        bool start();

        //- execute is called at each ++ or += of the time-loop
        virtual bool execute
        (
#if (defined(OF_FORK_extend) && OF_VERSION>=010604) || (!defined(OF_FORK_extend) && OF_VERSION<040000) //not (defined(OF16ext) || defined(OFdev)||defined(OFplus)||defined(OFesi1806)) || defined(Fx41)
	  bool
#endif
	);
	
	virtual bool write();

        //- Read and set the function object if its data has changed
        virtual bool read(const dictionary& dict);

#if OF_VERSION>=020100 || (defined(OF_FORK_extend) && OF_VERSION>=010604) //(!defined(OF16ext)||defined(Fx41)) && !defined(OF21x)
          //- Update for changes of mesh
        virtual void updateMesh(const mapPolyMesh& mpm);

        //- Update for changes of mesh
        virtual void movePoints(
    #if (defined(OF_FORK_extend) && OF_VERSION>=010604) //defined(Fx41)
            const pointField&
    #else
            const polyMesh& mesh
    #endif
           );
#endif
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
