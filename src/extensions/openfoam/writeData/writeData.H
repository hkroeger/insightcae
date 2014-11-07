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
    Foam::writeData

Group
    grpJobControlFunctionObjects

Description
    Watches for presence of the named file in the $FOAM_CASE directory
    and aborts the calculation if it is present.

    Currently the following action types are supported:
    - noWriteNow
    - writeNow
    - nextWrite

SourceFiles
    writeData.C
    IOwriteData.H

\*---------------------------------------------------------------------------*/

#ifndef writeData_H
#define writeData_H
//~ 
#include "NamedEnum.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class objectRegistry;
class dictionary;
class polyMesh;
class mapPolyMesh;

/*---------------------------------------------------------------------------*\
                      Class writeData Declaration
\*---------------------------------------------------------------------------*/

class writeData
{
private:

    // Private data

        //- Name of the abort file unless otherwise specified
        word name_;

        const objectRegistry& obr_;

        //- The fully-qualified name of the abort file
        fileName abortFile_;

    // Private Member Functions

        //- Remove write flag file.
        void removeFile() const;

        //- Disallow default bitwise copy construct
        writeData(const writeData&);

        //- Disallow default bitwise assignment
        void operator=(const writeData&);


public:

    //- Runtime type information
    TypeName("writeData");


    // Constructors

        //- Construct for given objectRegistry and dictionary.
        writeData
        (
            const word& name,
            const objectRegistry&,
            const dictionary&,
            const bool loadFromFilesUnused = false
        );


    //- Destructor
    virtual ~writeData();


    // Member Functions

        //- Return name of the abort file
        virtual const word& name() const
        {
            return name_;
        }

        //- Read the dictionary settings
        virtual void read(const dictionary&);

        //- Execute, check existence of abort file and take action
        virtual void execute();

        //- Execute at the final time-loop, used for cleanup
        virtual void end();

        //- Called when time was set at the end of the Time::operator++
        virtual void timeSet();

        //- Execute, check existence of abort file and take action
        virtual void write();

        //- Update for changes of mesh - does nothing
        virtual void updateMesh(const mapPolyMesh&)
        {}

        //- Update for changes of mesh - does nothing
        virtual void movePoints(const polyMesh&)
        {}
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
