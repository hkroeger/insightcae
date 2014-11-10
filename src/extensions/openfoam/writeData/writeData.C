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

\*---------------------------------------------------------------------------*/

#include "writeData.H"
#include "dictionary.H"
#include "error.H"
#include "Time.H"
#include "OSspecific.H"
#include "PstreamReduceOps.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
defineTypeNameAndDebug(writeData, 0);
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::writeData::removeFile() const
{
    bool hasAbort = isFile(abortFile_);
    reduce(hasAbort, orOp<bool>());

    if (hasAbort && Pstream::master())
    {
        // cleanup ABORT file (on master only)
        rm(abortFile_);
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::writeData::writeData
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict,
    const bool loadFromFiles
)
:
    name_(name),
    obr_(obr),
    abortFile_("$FOAM_CASE/" + name)
{
    abortFile_.expand();
    read(dict);

    // remove any old files from previous runs
    removeFile();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::writeData::~writeData()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::writeData::read(const dictionary& dict)
{
    if (dict.readIfPresent("fileName", abortFile_))
    {
        abortFile_.expand();
    }
}


void Foam::writeData::execute()
{
    bool write = isFile(abortFile_);
    reduce(write, orOp<bool>());

    if (write)
    {
#if defined(OF16ext) || defined(OF21x)
        const_cast<Time&>(obr_.time()).writeNow();
#else
        const_cast<Time&>(obr_.time()).writeOnce();
#endif
        Info<< "USER REQUESTED DATA WRITE AT (timeIndex="
            << obr_.time().timeIndex()
            << ")"
            << endl;
            
        removeFile();
    }
}

void Foam::writeData::updateMesh(const mapPolyMesh& mpm)
{
}

#if defined(OF16ext) || defined(OF21x)
void Foam::writeData::movePoints(const pointField& pf)
#else
void Foam::writeData::movePoints(const polyMesh& mesh)
#endif
{
}


void Foam::writeData::end()
{
    //removeFile();
}


void Foam::writeData::timeSet()
{
    // Do nothing - only valid on execute
}


void Foam::writeData::write()
{
    // Do nothing - only valid on execute
}


// ************************************************************************* //
