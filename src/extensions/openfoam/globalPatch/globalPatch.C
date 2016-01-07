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


#include "globalPatch.H" 

#include "transform.H"
#include "transformField.H"
#include "volFields.H"
#include "ListListOps.H"
#include "PstreamReduceOps.H"
#include "addToRunTimeSelectionTable.H"
#include "globalMeshData.H"
#include "globalIndex.H"

#include "base/vtktools.h"

#include <vector>
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;

namespace Foam 
{
  
label reduced(label l)
{
  reduce(l, sumOp<label>());
  return l;
}

  
autoPtr<PrimitivePatch<face, List, pointField> > globalPatch::createGlobalPatch(const polyPatch& patch)
{
  const polyMesh& mesh=patch.boundaryMesh().mesh();
  
  IOobject addrHeader
  (
      "pointProcAddressing",
      mesh.facesInstance()/mesh.meshSubDir,
      mesh,
      IOobject::MUST_READ
  );
  
  if (addrHeader.headerOk())
  {
    // There is a pointProcAddressing file so use it to get labels
    // on the original mesh
    labelIOList gpi(addrHeader);
  
    typedef Tuple2<point,label> piTuple;
    typedef HashTable<piTuple,label> pointHashTable;
    
    pointHashTable usedGlobalPts;
    forAll(patch.localPoints(), lpI)
    {
      label gpt=gpi[patch.meshPoints()[lpI]];
      usedGlobalPts.insert( gpt, piTuple(patch.localPoints()[lpI], -1) );
    }
    
    Pstream::mapCombineGather(usedGlobalPts, eqOp<piTuple>());
        
    if (Pstream::master())
    {
      label idx=0;
      forAllIter(pointHashTable, usedGlobalPts, pi)
      {
	pi().second()=idx++;
      }
    }

    Pstream::mapCombineScatter(usedGlobalPts);

    pointField globalPoints(usedGlobalPts.size());
    forAllIter(pointHashTable, usedGlobalPts, pi)
    {
      globalPoints[pi().second()] = pi().first();
    }
    
    List<faceList> allfaces(Pstream::nProcs());
    faceList& myfaces=allfaces[Pstream::myProcNo()];
    myfaces.resize(patch.size());
    
    forAll(patch, fI)
    {
      const face& f = patch[fI];
      face gf(f.size());
      forAll(gf, vI)
      {
	gf[vI] = usedGlobalPts[ gpi[ f[vI] ] ].second();
      }
      myfaces[fI]=gf;
    }
    
    Pstream::gatherList(allfaces);
    Pstream::scatterList(allfaces);
    
    faceList globalFaces =
	ListListOps::combine<faceList>
	(
	  allfaces, accessOp<faceList>()
	);
    
    return autoPtr<PrimitivePatch<face, List, pointField> >
    (
      new PrimitivePatch<face, List, pointField>(globalFaces, globalPoints)
    );
  }
  else
  {
    return autoPtr<PrimitivePatch<face, List, pointField> >
    (
      new PrimitivePatch<face, List, pointField>(patch.localFaces(), patch.localPoints())
    );
  }
  
}

  
globalPatch::globalPatch(const polyPatch& patch)
: PrimitivePatch<face, List, pointField>(createGlobalPatch(patch)()),
  procOfs_(Pstream::nProcs(), -1)
{
  labelList nfaces(Pstream::nProcs());
  nfaces[Pstream::myProcNo()]=patch.size();

  Pstream::gatherList(nfaces);
  Pstream::scatterList(nfaces);
  
  procOfs_[0]=0;
  nfaces_=nfaces[0];
  for(label j=1; j<Pstream::nProcs(); j++)
  {
    nfaces_+=nfaces[j];
    procOfs_[j]=procOfs_[j-1] + nfaces[j-1];
  }
}

}