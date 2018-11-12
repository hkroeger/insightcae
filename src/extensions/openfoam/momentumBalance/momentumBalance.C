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

#include "base/vtktools.h"

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

//template<class T>
//tmp<Field<T> > pick(const List<T>& data, const labelList& idxs, const scalarField* mult=NULL)
//{
//  tmp<Field<T> > tres(new Field<T>(idxs.size(), pTraits<T>::zero));
//  Field<T>& res = UNIOF_TMP_NONCONST(tres);

//  forAll(idxs, i)
//  {
//      res[i] = data[idxs[i]];

//      if (mult!=NULL) res[i] *= (*mult)[i];
//  }
//  return tres;
//}

template<class T>
tmp<Field<T> > pick_gf(const GeometricField<T, fvsPatchField, surfaceMesh>& data, const labelList& idxs, const scalarField* mult=NULL)
{
  tmp<Field<T> > tres(new Field<T>(idxs.size(), pTraits<T>::zero));
  Field<T>& res = UNIOF_TMP_NONCONST(tres);

  forAll(idxs, i)
  {
      label fi=idxs[i];

      if (fi<data.mesh().nInternalFaces())
      {
        res[i] = data[idxs[i]];
      } else
      {
        label pi=data.mesh().boundaryMesh().whichPatch(fi);
        res[i] = data.boundaryField()[pi][fi-data.mesh().boundaryMesh()[pi].start()];
      }

      if (mult!=NULL) res[i] *= (*mult)[i];
  }
  return tres;
}


void createVTKGeometry
(
    const fvMesh& mesh,
    const labelList& faces,
    insight::vtk::vtkModel2d& vtk
)
{
  std::map<label, label> locPtIndex; // global pointindex => local index

  //create entry for each used pt
  forAll(faces, i)
  {
    label fi=faces[i];
    const face& f = mesh.faces()[fi];
    forAll(f, j) locPtIndex[f[j]]=-1;
  }

  // locally number pt
  label k=0;
  for (auto& j: locPtIndex) j.second=k++;

  //insert into vtk
  double x[k], y[k], z[k];
  k=0;
  for (const auto& j: locPtIndex)
    {
      x[k]=mesh.points()[j.first].x();
      y[k]=mesh.points()[j.first].y();
      z[k]=mesh.points()[j.first].z();
      k++;
    }
  vtk.setPoints(k, x, y, z);

  // insert faces with renumbered vertices
  forAll(faces, i)
  {
    label fi=faces[i];
    const face& f = mesh.faces()[fi];
    int ci[f.size()];
    forAll(f, j)
    {
      ci[j]=locPtIndex[f[j]];
    }
    vtk.appendPolygon(f.size(), ci);
  }
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
    scalar rhoInf = readScalar(IStringStream(UNIOF_ADDARG(args,1))());

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        mesh.readUpdate();

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
	IOobject rhoheader
	(
	    "rho",
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

//	  Info<<bndfaces<<norm_dir<<endl;

	  volVectorField U(Uheader, mesh);
	  volScalarField p(pheader, mesh);
	  surfaceScalarField phi(phiheader, mesh);

	  autoPtr<volScalarField> rho;
	  if (UNIOF_HEADEROK(rhoheader,volScalarField))
	    {
	      rho.reset(new volScalarField(rhoheader, mesh));
	    }
	  else
	    {
	      Info<<"Using rhoInf="<<rhoInf<<endl;

	      rho.reset(
		    new volScalarField
		    (
		      IOobject
		      (
			"rho",
			runTime.timeName(),
			mesh,
			IOobject::NO_READ,
			IOobject::NO_WRITE
		      ),
		      mesh,
		      dimensionedScalar("", dimDensity, rhoInf)
		    )
	      );
	    }

	  vectorField Sf=pick_gf(mesh.Sf(), bndfaces, &norm_dir);
	  scalarField phif=pick_gf(phi, bndfaces, &norm_dir);
	  vectorField Uf=pick_gf(fvc::interpolate(U)(), bndfaces);
	  scalarField rhof=pick_gf(fvc::interpolate(rho())(), bndfaces);
	  scalarField pf=pick_gf(fvc::interpolate(p)(), bndfaces);
	  if (p.dimensions()==dimPressure/dimDensity)
	    {
	      Info<<"Converting pressure."<<endl;
	      pf*=rhof;
	    }

	  {
	    vectorField Cf=pick_gf(mesh.Cf(), bndfaces);

	    insight::vtk::vtkModel2d vtk;
	    createVTKGeometry(mesh, bndfaces, vtk);
	    vtk.appendCellVectorField(
		   "Cf",
		   Cf.component(0)().cdata(),
		   Cf.component(1)().cdata(),
		   Cf.component(2)().cdata()
		  );
	    vtk.appendCellVectorField(
		   "Sf",
		   Sf.component(0)().cdata(),
		   Sf.component(1)().cdata(),
		   Sf.component(2)().cdata()
		  );
	    vtk.appendCellVectorField(
		   "Uf",
		   Uf.component(0)().cdata(),
		   Uf.component(1)().cdata(),
		   Uf.component(2)().cdata()
		  );
	    vtk.appendCellScalarField(
		   "p",
		   pf.cdata()
		  );
	    vtk.appendCellScalarField(
		   "phi",
		   phif.cdata()
		  );
	    vtk.createLegacyFile
		(
		  IOobject
		  (
		    cellSetName+"_visualization.vtk",
		    runTime.timeName(),
		    runTime,
		    IOobject::NO_READ,
		    IOobject::NO_WRITE
		  ).objectPath()
		);
	  }

	  vector F=gSum(

		rhof*phif*Uf + pf*Sf

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
