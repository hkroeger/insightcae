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
    List<surfaceScalarField*> blendingFactors_;

    //- Reference to main object registry
    const Time& time_;

    //- Region name
    word regionName_;

    bool markNonOrthFaces_;
    bool markSkewFaces_;
    bool markWarpedFaces_;
    bool markConcaveFaces_;
    bool markHighAspectFaces_;
    bool markLowQualityTetFaces_;
    bool markVelocityPeaks_;
    bool smoothMarkerField_;
    scalar aspectThreshold_;
    scalar lowerNonOrthThreshold_;
    scalar upperNonOrthThreshold_;
    scalar smoothingCoeff_;
    wordList sets_;

    const polyMesh& mesh_;
    wordList blendingFieldNames_;

// Private Member Functions


    void markFaceSet(const faceSet& faces);
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
        virtual bool start();

        //- execute is called at each ++ or += of the time-loop
        virtual bool execute
        (
#ifndef OF16ext
	  bool
#endif
	);

        //- Read and set the function object if its data has changed
        virtual bool read(const dictionary& dict);

#if !defined(OF16ext) && !defined(OF21x)
          //- Update for changes of mesh
        virtual void updateMesh(const mapPolyMesh& mpm);

        //- Update for changes of mesh
        virtual void movePoints(const polyMesh& mesh);
#endif
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
