/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::functionObject

Description
    Abstract base-class for Time/database function objects.

See Also
    Foam::OutputFilterFunctionObject

SourceFiles
    functionObject.C

\*---------------------------------------------------------------------------*/

#ifndef twoPointCorrelation_H
#define twoPointCorrelation_H

#include "typeInfo.H"
#include "autoPtr.H"
#include "runTimeSelectionTables.H"

#include "OFstream.H"
#include "cloudSet.H"
#include "sampledSets.H"
#include "meshSearch.H"
#include "coordinateSystem.H"

#include "OutputFilterFunctionObject.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#include <boost/concept_check.hpp>

namespace Foam
{

// Forward declaration of classes
class Time;
class polyMesh;
class mapPolyMesh;

/*---------------------------------------------------------------------------*\
                       Class functionObject Declaration
\*---------------------------------------------------------------------------*/


//- Class used for sampling volFields
template<class Type>
class volFieldSampler
: public List<Field<Type> >
{
    //- Name of this collection of values
    const word name_;

public:

    //- Construct interpolating field to the sampleSets
    volFieldSampler
    (
        const word& interpolationScheme,
        const GeometricField<Type, fvPatchField, volMesh>& field,
        const PtrList<sampledSet>& samplers
    )
        :
        List<Field<Type> >(samplers.size()),
        name_(field.name())
    {
        autoPtr<interpolation<Type> > interpolator
        (
            interpolation<Type>::New(interpolationScheme, field)
        );

        forAll(samplers, setI)
        {
            Field<Type>& values = this->operator[](setI);
            const sampledSet& samples = samplers[setI];

            values.setSize(samples.size());
            forAll(samples, sampleI)
            {
                const point& samplePt = samples[sampleI];
                label cellI = samples.cells()[sampleI];
                label faceI = samples.faces()[sampleI];

                if (cellI == -1 && faceI == -1)
                {
                    // Special condition for illegal sampling points
                    values[sampleI] = pTraits<Type>::max;
                }
                else
                {
                    values[sampleI] = interpolator().interpolate
                                      (
                                          samplePt,
                                          cellI,
                                          faceI
                                      );
                }
            }
        }
    }

    volFieldSampler
    (
        const List<Field<Type> >& values,
        const word& name
    )
        :
        List<Field<Type> >(values),
        name_(name)
    {}

    inline const word& name() const {
        return name_;
    }
};


class twoPointCorrelation
{

protected:
    word name_;
	
    const objectRegistry& obr_;

    //- on/off switch
    bool active_;
    
    //- Mesh search engine
    autoPtr<meshSearch> searchEngine_;
    
    autoPtr<OFstream> filePtr_;

    point p0_;
    vector directionSpan_;
    label np_;
    
    vector homogeneousTranslationUnit_;
    label nph_;
    
    autoPtr<coordinateSystem> csys_;
    
    PtrList<sampledSet> lines_;
    
    // Merging structures
    autoPtr<scalarField> x_;
    PtrList<coordSet> masterSampledSets_;
    labelListList indexSets_;

    autoPtr<tensorField> correlationCoeffs_;
    scalar totalTime_;

    //- If the forces file has not been created create it
    void makeFile();

    //- Output file header information
    virtual void writeFileHeader();
    
    void createInterpolators();
    
    void combineSampledSets
    (
	PtrList<coordSet>& masterSampledSets,
	labelListList& indexSets
    );


public:
    //- Runtime type information
    TypeName("twoPointCorrelation");

    // Constructors
    twoPointCorrelation
    (
      const word& name,
      const objectRegistry&,
      const dictionary&,
      const bool loadFromFiles = false
    );
    virtual ~twoPointCorrelation();

    // Member Functions
    virtual const word& name() const { return name_; }

    //- Read and set the function object if its data have changed
    virtual void read(const dictionary&);

    virtual void start();
    virtual void execute();
    virtual void end();

    //- Called when time was set at the end of the Time::operator++
    virtual void timeSet();

    virtual void write();

    //- Update for changes of mesh
    virtual void updateMesh(const mapPolyMesh& mpm);
    virtual void movePoints(const polyMesh& mesh);    
    virtual void readUpdate(const polyMesh::readUpdateState state);
};

typedef OutputFilterFunctionObject<twoPointCorrelation>
    twoPointCorrelationFunctionObject;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
