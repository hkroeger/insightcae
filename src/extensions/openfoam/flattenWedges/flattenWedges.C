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
#include "OFstream.H"
#include "transformField.H"
#include "transformGeometricField.H"
#include "wordReList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

void flattenPatch(pointIOField& points, polyMesh& mesh, const word& patchname, const point& p0)
{
  label pi=mesh.boundaryMesh().findPatchID(patchname);
  
  const polyPatch& patch=mesh.boundaryMesh()[pi];
  vector normal=gAverage(patch.faceNormals());
  normal/=mag(normal);
  
  scalar corr=0;
  forAll(patch.meshPoints(), mpi)
  {
    label pi=patch.meshPoints()[mpi];
    point& p=points[pi];
    vector off=normal*((p-p0)&normal);
    corr+=mag(off);
    p-=off;
  }
  corr/=scalar(patch.meshPoints().size());
  
  Info<<"patch "<<patchname<<": average correction = "<<corr<<endl;
}

int main(int argc, char *argv[])
{

    argList::validArgs.append("p0");
    argList::validArgs.append("wedgepatch1");
    argList::validArgs.append("wedgepatch2");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createPolyMesh.H"
    
    pointIOField points
    (
        IOobject
        (
            "points",
            runTime.findInstance(polyMesh::meshSubDir, "points"),
            polyMesh::meshSubDir,
            runTime,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
        )
    );

    boundBox bb(points);

    Info<< "bounding box: min = " << bb.min()
        << " max = " << bb.max() << " metres."
        << endl;
    
    point p0(IStringStream(args.additionalArgs()[0])());
    word wp1(IStringStream(args.additionalArgs()[1])());
    word wp2(IStringStream(args.additionalArgs()[2])());
    
    flattenPatch(points, mesh, wp1, p0);
    flattenPatch(points, mesh, wp2, p0);
    
    // Set the precision of the points data to 10
    IOstream::defaultPrecision(max(10u, IOstream::defaultPrecision()));

    Info<< "Writing points into directory " << points.path() << nl << endl;
    points.write();

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
