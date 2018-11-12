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
#include "OFstream.H"
#include "syncTools.H"

#include "uniof.h"

using namespace Foam;

void findBndFaces(const fvMesh& mesh, const cellSet& cells, labelList& res, scalarField& normal_direction)
{
  const label nInt = mesh.nInternalFaces();
  const labelList& own = mesh.faceOwner();
  const labelList& nei = mesh.faceNeighbour();
  const polyBoundaryMesh& patches = mesh.boundaryMesh();

  labelHashSet faces;

  // Add all faces from cell
  forAllConstIter(cellSet, cells, iter)
  {
      const label celli = iter.key();
      const labelList& cFaces = mesh.cells()[celli];

      forAll(cFaces, cFacei)
      {
          faces.insert(cFaces[cFacei]);
      }
  }


  // Check all internal faces
  for (label facei = 0; facei < nInt; facei++)
  {
      if (cells.found(own[facei]) && cells.found(nei[facei]))
      {
          faces.erase(facei);
      }
  }

  // Get coupled cell status
  boolList neiInSet(mesh.nFaces()-nInt, false);

  forAll(patches, patchi)
  {
      const polyPatch& pp = patches[patchi];

      if (pp.coupled())
      {
          label facei = pp.start();
          forAll(pp, i)
          {
              neiInSet[facei-nInt] = cells.found(own[facei]);
              facei++;
          }
      }
  }
  syncTools::swapBoundaryFaceList(mesh, neiInSet);

  // Check all boundary faces
  forAll(patches, patchi)
  {
      const polyPatch& pp = patches[patchi];

      if (pp.coupled())
      {
          label facei = pp.start();
          forAll(pp, i)
          {
              if (cells.found(own[facei]) && neiInSet[facei-nInt])
              {
                  faces.erase(facei);
              }
              facei++;
          }
      }
  }


  res.setSize(faces.size());
  normal_direction.setSize(faces.size());
  label j=0;
  forAllConstIter(labelHashSet, faces, i)
  {
    label fi=i.key();

    res[j]=fi;
    if (cells.found(own[fi]))
      {
        normal_direction[j]=1.0;
      }
    else if (cells.found(nei[fi]))
      {
        normal_direction[j]=-1.0;
      }
    else
      {
        std::cerr << "Häää???"<<std::endl;
      }

    j++;
  }

}

template<class T>
tmp<Field<T> > pick(const List<T>& list, const labelList& idxs, const scalarField* mult=NULL)
{
  tmp<Field<T> > tres(new Field<T>(idxs.size(), pTraits<T>::zero));
  Field<T>& res = UNIOF_TMP_NONCONST(tres);

  forAll(idxs, i)
  {
      res[i]=list[idxs[i]];
      if (mult) res[i]*=(*mult)[i];
  }
  return tres;
}



int main(int argc, char *argv[])
{
    timeSelector::addOptions();

    argList::validArgs.append("cellSet");
    argList::validArgs.append("density [kg/m^3]");

    #include "setRootCase.H"
    #include "createTime.H"
    instantList timeDirs = timeSelector::select0(runTime, args);
    #include "createNamedMesh.H"


    word cellSetName(IStringStream(UNIOF_ADDARG(args,0))());
    scalar rho = readScalar(IStringStream(UNIOF_ADDARG(args,1))());

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        /*fvMesh::readUpdateState state = */ mesh.readUpdate();

	IOobject Uheader
	(
	    "U",
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::NO_WRITE
	);
	IOobject pheader
	(
	    "p",
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::NO_WRITE
	);
	IOobject phiheader
	(
	    "phi",
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::NO_WRITE
	);

	if (UNIOF_HEADEROK(Uheader,volVectorField) && UNIOF_HEADEROK(pheader,volScalarField) && UNIOF_HEADEROK(phiheader,surfaceScalarField))
	{
	  cellSet cells(mesh, cellSetName, cellSet::MUST_READ);

	  labelList bndfaces;
	  scalarField norm_dir;
	  findBndFaces(mesh, cells, bndfaces, norm_dir);

	  volVectorField U(Uheader, mesh);
	  volScalarField p(pheader, mesh);
	  surfaceScalarField phi(phiheader, mesh);

	  vectorField Sf=pick(Sf, bndfaces, &norm_dir);
	  vectorField Uf=pick(fvc::interpolate(U)(), bndfaces);
	  scalarField pf=pick(fvc::interpolate(p)(), bndfaces);
	  scalarField phif=pick(phi, bndfaces, &norm_dir);

	  vector F=gSum(

		rho* ( phif*Uf + pf*Sf )

		);

	  Pout<<"F="<<F<<endl;
	}
	else
	{
	  FatalErrorIn("main") << "All fields U, p and phi have to be present!" << abort(FatalError);
	}
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
