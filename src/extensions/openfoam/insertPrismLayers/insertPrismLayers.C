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

Application
    refineWallLayer

Description
    Utility to refine cells next to patches.

    Takes a patchName and number of layers to refine. Works out cells within
    these layers and refines those in the wall-normal direction.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#ifdef Fx40
#include "foamTime.H"
#else
#include "Time.H"
#endif
#if defined(OF16ext)
#include "directTopoChange.H"
#else
#include "polyTopoChange.H"
#include "polyTopoChanger.H"
#endif
#include "mapPolyMesh.H"
#include "polyMesh.H"
#include "cellCuts.H"
#include "cellSet.H"
#include "meshCutter.H"

#include "polyAddPoint.H"
#include "polyAddCell.H"
#include "polyAddFace.H"


using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    Foam::argList::validOptions.insert("overwrite", "");
    argList::noParallel();
    argList::validArgs.append("patchName");
    argList::validArgs.append("wallDist");

    Foam::argList::validOptions.insert("useSet", "cellSet");


    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
