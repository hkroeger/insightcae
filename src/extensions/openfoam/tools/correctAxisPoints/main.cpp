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

#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{

    argList::validArgs.append("p0");
    argList::validArgs.append("axis");
    argList::validArgs.append("tol");

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

    point p0(IStringStream( UNIOF_ADDARG(args, 0) )());
    vector ax(IStringStream( UNIOF_ADDARG(args, 1) )());
    scalar tol=readScalar(IStringStream( UNIOF_ADDARG(args, 2) )());

    label ncorr=0;
    scalar dmax=0;
    forAll(points, pi)
    {
        point& p = points[pi];
        vector r = p-p0;
        vector dr = r - ax*(r&ax);
        if (mag(dr)<tol)
        {
            p -= dr;
            ncorr++;
            dmax = max(mag(dr), dmax);
        }
    }
    Info<<"Corrected "<<ncorr<<" points, maximum correction magnitude = "<<dmax<<endl;


    // Set the precision of the points data to 10
    IOstream::defaultPrecision(max(10u, IOstream::defaultPrecision()));

    Info<< "Writing points into directory " << points.path() << nl << endl;
    points.write();

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
