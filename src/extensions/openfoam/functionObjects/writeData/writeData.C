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

#include "OSspecific.H"
#include "PstreamReduceOps.H"

#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
defineTypeNameAndDebug(writeData, 0);
addToRunTimeSelectionTable
(
    functionObject,
    writeData,
    dictionary
);
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::writeData::removeFile() const
{
    bool hasWrite = isFile(writeFile_);
    reduce(hasWrite, orOp<bool>());
    
    bool hasAbort = isFile(abortFile_);
    reduce(hasAbort, orOp<bool>());

    if (hasWrite && Pstream::master())
    {
        // cleanup ABORT file (on master only)
        rm(writeFile_);
    }
    
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
    const Time& obr,
    const dictionary& dict
)
:
  UniFunctionObject(name),
    name_(name),
    obr_(obr),
    writeFile_("$FOAM_CASE/" + name),
    abortFile_("$FOAM_CASE/" + name+"Abort")
{}




// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::writeData::read(const dictionary& dict)
{
    if (dict.readIfPresent("fileName", writeFile_))
    {
        writeFile_.expand();
    }
    
    if (dict.readIfPresent("fileNameAbort", abortFile_))
    {
        abortFile_.expand();
    }

    return true;
}


bool Foam::writeData::perform()
{
    bool write = isFile(writeFile_);
    reduce(write, orOp<bool>());
    bool abort = isFile(abortFile_);
    reduce(abort, orOp<bool>());

    if (write||abort)
    {    
        removeFile();
    }

    if (write)
    {
      Info<< "USER REQUESTED DATA WRITE AT (timeIndex="
          << obr_.time().timeIndex()
          << ")"
          << endl;

#if OF_VERSION<=020100 //defined(OF16ext) || defined(OF21x)
      const_cast<Time&>(obr_.time()).writeNow();
#else
      const_cast<Time&>(obr_.time()).writeOnce();
#endif
    }
    
    if (abort)
    {
      Info<< "USER REQUESTED ABORT (timeIndex="
          << obr_.time().timeIndex()
          << "): stop+write data"
          << endl;

#if OF_VERSION<010700 //defined(OF16ext) //defined(OF21x)
      const_cast<Time&>(obr_.time()).writeAndEnd();
#else
      const_cast<Time&>(obr_.time()).stopAt(Time::saWriteNow);
#endif
    }

    return true;
}




bool Foam::writeData::write()
{
   return false;
}


// ************************************************************************* //
