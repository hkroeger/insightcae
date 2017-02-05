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

#include "uniof.h"


template<class T>
T getGlobalFaceValue(const GeometricField<T, fvsPatchField, surfaceMesh>& f, label gfi)
{
  const fvMesh& mesh = f.mesh();
  const fvPatchList & patches = mesh.boundary();
  
  if (gfi < mesh.nInternalFaces())
  {
    return f[gfi];
  }
  else
  {
    label pi = mesh.boundaryMesh().whichPatch(gfi);
//     Info<<"pi="<<pi<<endl;
    const fvPatch & curPatch = patches[pi];
    
    if (curPatch.type() == "empty") return pTraits<T>::zero;
    
    label start = curPatch.patch().start();
    label localfacei=gfi-start;
//     Info<<start<<" "<<localfacei<<" "<<curPatch.size()<<endl;
    return f.boundaryField()[pi][localfacei];
  }
  FatalErrorIn("getGlobalFaceValue") << "invalid global face ID: "<<gfi<<abort(FatalError);
  return pTraits<T>::zero;
}

int main(int argc, char *argv[])
{
    timeSelector::addOptions();
    
    argList::validArgs.append("list of (identifier, (STL file name, outside point))");
    argList::validArgs.append("density [kg/m^3]");
//     argList::validArgs.append("static pressure p0 [Pa]");
    
    #include "setRootCase.H"
    #include "createTime.H"
    instantList timeDirs = timeSelector::select0(runTime, args);
    #include "createNamedMesh.H"

//     const bool compressible = args.optionFound("compressible");
    typedef Tuple2<fileName, point> setInfo;
    typedef HashTable<setInfo> setInfoMap;
    setInfoMap setInfos(IStringStream(UNIOF_ADDARG(args,0))());
    
    scalar rho = readScalar(IStringStream(UNIOF_ADDARG(args,1))());
//     scalar p0 = readScalar(IStringStream(UNIOF_ADDARG(args,2))());

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        fvMesh::readUpdateState state = mesh.readUpdate();

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
	
        if (UNIOF_HEADEROK(Uheader,volVectorField) && UNIOF_HEADEROK(pheader,volScalarField))
	{
	  volVectorField U(Uheader, mesh);
	  volScalarField p(pheader, mesh);
	  
	  surfaceVectorField Uf=fvc::interpolate(U);
	  surfaceScalarField pf=fvc::interpolate(p);
	
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
	    
	    cellSet cells(mesh, id+"_cells", 0);
	    stc.applyToSet(topoSetSource::NEW, cells);
	    Info<<"found "<<cells.size()<<" cells for "<<id<<endl;
	    cells.write();
	    
	    faceSet bndfaces(mesh, id+"_faces", 0);
	    cellToFace ctf1(mesh, id+"_cells", cellToFace::ALL);
	    ctf1.applyToSet(topoSetSource::NEW, bndfaces);
	    bndfaces.write();
	    cellToFace ctf2(mesh, id+"_cells", cellToFace::BOTH);
	    ctf2.applyToSet(topoSetSource::DELETE, bndfaces);
	    bndfaces.write();
	    Info<<"found "<<bndfaces.size()<<" bounding faces for "<<id<<endl;
	    
	    vectorField faceCenters(bndfaces.size(), vector::zero);
	    vectorField faceArea(bndfaces.size(), vector::zero);
	    vectorField velocity(bndfaces.size(), vector::zero);
	    scalarField pressure(bndfaces.size(), 0);
	    label k=-1;
	    forAllConstIter(faceSet, bndfaces, j)
	    {
	      k+=1;
	      label faceI=j.key();
// 	      Info<<"faceI="<<faceI<<endl;
	      
	      faceCenters[k]=mesh.faceCentres()[faceI];
	      if (faceI>=mesh.nInternalFaces())
	      {
		faceArea[k]=mesh.faceAreas()[faceI];
	      }
	      else
	      {
		label ownci = mesh.owner()[faceI];
		label neici = mesh.neighbour()[faceI];
		bool hasown=cells.found(ownci);
		bool hasnei=cells.found(neici);
		if (hasown && !hasnei)
		{
		  faceArea[k]=mesh.faceAreas()[faceI];
		}
		else if (hasnei && !hasown)
		{
		  faceArea[k]=-mesh.faceAreas()[faceI];
		}
		else
		{
		  FatalErrorIn("main") << "Internal error: neither owner nor neighbour cell in cell set!" << abort(FatalError);
		}
	      }
	      
	      velocity[k]=getGlobalFaceValue(Uf, faceI);
	      pressure[k]=getGlobalFaceValue(pf, faceI);
	    }

	    {
	      OFstream f(id+".csv");
	      f<<"# id,x,y,z,ux,uy,uz,p,phi,sfx,sfy,sfz"<<endl;
	      forAll(pressure, l)
	      {
		f
		  <<l<<","
		  <<faceCenters[l][0]<<","
		  <<faceCenters[l][1]<<","
		  <<faceCenters[l][2]<<","
		  <<velocity[l][0]<<","
		  <<velocity[l][1]<<","
		  <<velocity[l][2]<<","
		  <<pressure[l]<<","
		  <<(faceArea[l]&velocity[l])<<","
		  <<faceArea[l][0]<<","
		  <<faceArea[l][1]<<","
		  <<faceArea[l][2]
		  <<endl;
	      }
	    }
	    scalarField integrand = rho*(0.5*magSqr(velocity)+pressure) * (velocity&faceArea);
// 	    Info<<integrand<<endl;
	    scalar Wt = gSum( integrand );
	    Info<<"volume integrated energy loss at "<<id<<": Wt = "<<Wt<<endl;
	    Info<<"volume flux error at "<<id<<": delta V = "<<gSum(velocity&faceArea) <<endl;
	  }
	}
	else
	{
	  FatalErrorIn("main") << "Both fields U and p have to be present!" << abort(FatalError);
	}
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
