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

#include "fvCFD.H"
#include "Tuple2.H"
#include "surfaceToCell.H"
#include "cellSet.H"
#include "faceSet.H"
#include "cellToFace.H"

int main(int argc, char *argv[])
{
    timeSelector::addOptions();

// #include "addRegionOption.H"

//     argList::addBoolOption
//     (
//         "compressible",
//         "calculate compressible y+"
//     );
    
    argList::validArgs.append("list of (identifier, (STL file name, outside point))");
    
    #include "setRootCase.H"
    #include "createTime.H"
    instantList timeDirs = timeSelector::select0(runTime, args);
    #include "createNamedMesh.H"

//     const bool compressible = args.optionFound("compressible");
    typedef Tuple2<fileName, point> setInfo;
    typedef HashTable<setInfo> setInfoMap;
    setInfoMap setInfos(IStringStream(
#ifdef OFdev
      args.arg(1)
#else
      args.additionalArgs()[0]
#endif
    )());

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        fvMesh::readUpdateState state = mesh.readUpdate();
	
	forAllConstIter(setInfoMap, setInfos, i)
	{
	  word id = i.key();
	  fileName sfn = i().first();
	  point outpnt = i().second();
	  pointField outpnts(1, outpnt);
	  surfaceToCell stc(mesh, sfn, outpnts, false, true, false, 
#ifndef Fx40
			    false, 
#endif
			1e-4, 0);
	  
	  cellSet cells(mesh, id, 0);
	  stc.applyToSet(topoSetSource::NEW, cells);
	  Info<<"found "<<cells.size()<<" cells for "<<id<<endl;
	  
	  faceSet bndfaces(mesh, id, 0);
	  cellToFace ctf1(mesh, id, cellToFace::ALL);
	  ctf1.applyToSet(topoSetSource::NEW, bndfaces);
	  cellToFace ctf2(mesh, id, cellToFace::BOTH);
	  ctf2.applyToSet(topoSetSource::DELETE, bndfaces);
	  Info<<"found "<<bndfaces.size()<<" bounding faces for "<<id<<endl;
	}

//         // Wall distance
//         if (timeI == 0 || state != fvMesh::UNCHANGED)
//         {
//             Info<< "Calculating wall distance\n" << endl;
//             wallDist y(mesh, true);
//             Info<< "Writing wall distance to field " << y.name() << nl << endl;
//             y.write();
//         }
// 
//         volScalarField yPlus
//         (
//             IOobject
//             (
//                 "yPlus",
//                 runTime.timeName(),
//                 mesh,
//                 IOobject::NO_READ,
//                 IOobject::NO_WRITE
//             ),
//             mesh,
//             dimensionedScalar("yPlus", dimless, 0.0)
//         );
// 
//         IOobject UHeader
//         (
//             "U",
//             runTime.timeName(),
//             mesh,
//             IOobject::MUST_READ,
//             IOobject::NO_WRITE
//         );
// 
// #if defined(OFplus)
//         if (UHeader.typeHeaderOk<volVectorField>())
// #else
//         if (UHeader.headerOk())
// #endif
//         {
//             Info<< "Reading field U\n" << endl;
//             volVectorField U(UHeader, mesh);
// 
// //             if (compressible)
// //             {
// //                 calcCompressibleYPlus(mesh, runTime, U, yPlus);
// //             }
// //             else
//             {
//                 calcIncompressibleYPlus(mesh, runTime, U, yPlus);
//             }
//         }
//         else
//         {
//             Info<< "    no U field" << endl;
//         }
// 
//         Info<< "Writing yPlus to field " << yPlus.name() << nl << endl;
// 
//         yPlus.write();
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
