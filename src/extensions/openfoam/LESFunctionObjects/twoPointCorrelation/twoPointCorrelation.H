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
    
    vector homogeneousTranslationSpan_;
    label nph_;
    
    autoPtr<coordinateSystem> csys_;
    
    PtrList<cloudSet> lines_;
    autoPtr<tensorField> correlationCoeffs_;
    scalar totalTime_;

    //- If the forces file has not been created create it
    void makeFile();

    //- Output file header information
    virtual void writeFileHeader();
    void createInterpolators();

public:
    //- Runtime type information
    TypeName("twoPointCorrelation");

    // Constructors

        //- Construct from components
        twoPointCorrelation
        (
	  const word& name,
	  const objectRegistry&,
	  const dictionary&,
	  const bool loadFromFiles = false
	);



    //- Destructor
    virtual ~twoPointCorrelation();


    // Member Functions
    
        //- Return name of the set of staticPressure
        virtual const word& name() const
        {
            return name_;
        }



        //- Called at each ++ or += of the time-loop. forceWrite overrides the
        //  outputControl behaviour.
        virtual void execute();
	
        virtual void start();
        virtual void end();

        //- Called when time was set at the end of the Time::operator++
        virtual bool timeSet();

        //- Read and set the function object if its data have changed
        virtual void read(const dictionary&);
	
        virtual void write();

        //- Update for changes of mesh
        virtual void updateMesh(const mapPolyMesh& mpm);

        //- Update for changes of mesh
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
