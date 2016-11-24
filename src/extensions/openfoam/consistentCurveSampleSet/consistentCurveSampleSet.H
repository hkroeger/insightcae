/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     |
    \\  /    A nd           | For copyright notice see file Copyright
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::consistentCurveSet

Description

SourceFiles
    consistentCurveSet.C

\*---------------------------------------------------------------------------*/

#ifndef consistentCurveSet_H
#define consistentCurveSet_H

#include "sampledSet.H"
#include "DynamicList.H"

#ifndef OF16ext
#include "passiveParticleCloud.H"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class passiveParticle;
template<class Type> class Particle;

/*---------------------------------------------------------------------------*\
                           Class consistentCurveSet Declaration
\*---------------------------------------------------------------------------*/

class consistentCurveSet
:
    public sampledSet
{
    // Private data

        //- sampling points
        List<point> sampleCoords_;
	point basept_;


    // Private Member Functions

        //- Sample till hits boundary. Called with singleParticle at position
        //  inbetween sampleCoords_[sampleI] and sampleCoords_[sampleI+1].
        //  Returns false if end of samples reached.
        bool trackToBoundary
        (
#ifdef OF16ext
            Particle<passiveParticle>& singleParticle,
#elif defined (OF21x)||defined (OF22x)||defined (OF22eng)
            passiveParticle& singleParticle,
#else
            passiveParticleCloud& particleCloud,
            passiveParticle& singleParticle,
#endif
            label& sampleI,
            DynamicList<point>& samplingPts,
            DynamicList<label>& samplingCells,
            DynamicList<label>& samplingFaces,
            DynamicList<scalar>& samplingCurveDist
        ) const;

        //- Samples all point in sampleCoords_
        //  samplingSegments contains segmentNo for each sample.
        void calcSamples
        (
            DynamicList<point>& samplingPts,
            DynamicList<label>& samplingCells,
            DynamicList<label>& samplingFaces,
            DynamicList<label>& samplingSegments,
            DynamicList<scalar>& samplingCurveDist
        ) const;

        //- Uses calcSamples to obtain samples. Copies them into *this.
        void genSamples();


public:

    //- Runtime type information
    TypeName("consistentCurve");

#if defined(OFplus)
    // Static data

        //- Tolerance when comparing points relative to difference between
        //  start_ and end_
        static const scalar tol;
#endif
	
    // Constructors

        //- Construct from components
        consistentCurveSet
        (
            const word& name,
            const polyMesh& mesh,
#if defined(OF16ext)||defined(OF21x)
            meshSearch& searchEngine,
#else
            const meshSearch& searchEngine,
#endif
            const word& axis,
            const List<point>& samplePoints
        );

        //- Construct from dictionary
        consistentCurveSet
        (
            const word& name,
            const polyMesh& mesh,
#if defined(OF16ext)||defined(OF21x)
            meshSearch& searchEngine,
#else
            const meshSearch& searchEngine,
#endif
            const dictionary& dict
        );


    // Destructor

        virtual ~consistentCurveSet();


    // Member Functions

        //- Get reference point
        virtual point getRefPoint(const List<point>&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
