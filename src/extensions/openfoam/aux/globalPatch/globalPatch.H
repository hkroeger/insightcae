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

#ifndef globalPatch_H
#define globalPatch_H

// #include "boostRandomGen.H"
#include "fixedValueFvPatchFields.H"

#include "SLList.H"
#include "Switch.H"
#include "ListListOps.H"
#include "PstreamReduceOps.H"
#include "Tuple2.H"

#include "indexedOctree.H"
#include "treeDataPoint.H"
#include "treeDataFace.H"

#include "PatchTools.H"

#include "uniof.h"

// #include "fielddataprovider.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

label reduced(label l);
  
  
template<class T>
label globalFieldSize(const Field<T>& f)
{
  label s=f.size();
  reduce(s, sumOp<label>());
  return s;
}
  
template<class T>
class GlobalField
: public Field<T >
{
  List<label> localSizes_;
  List<Field<T> > localFields_;
  
  void syncSizes()
  {
    localSizes_[Pstream::myProcNo()]=localFields_[Pstream::myProcNo()].size();
    Pstream::gatherList(localSizes_);
    Pstream::scatterList(localSizes_);   
    this->Field<T>::resize(sum(localSizes_));
  }
  
public:
  
  GlobalField()
  : localSizes_(Pstream::nProcs()),
    localFields_(Pstream::nProcs())
  {}
  
  GlobalField(label localsize)
  : localSizes_(Pstream::nProcs()),
    localFields_(Pstream::nProcs())
  {
    resize(localsize);
  }
  
  GlobalField(const Field<T>& localfield)
  : localSizes_(Pstream::nProcs()),
    localFields_(Pstream::nProcs())
  {
    insertLocalValues(localfield);
  }
  
  void resize(label localsize)
  {
    localFields_[Pstream::myProcNo()].resize(localsize);
    localFields_[Pstream::myProcNo()]=pTraits<T>::zero;
    syncSizes();
  }
  
  const Field<T>& getLocalValues() const
  {
//     syncGlobalSumToLocal();
    return localFields_[Pstream::myProcNo()];
  }

  void setLocalValue(label localI, const T& value)
  {
    localFields_[Pstream::myProcNo()][localI]=value;
  }

  void insertLocalValues(const Field<T>& localfield)
  {
    localFields_[Pstream::myProcNo()]=localfield;
    syncSizes();
    syncLocalToGlobal();
  }
  
  void syncGlobalSumToLocal()
  {
    Pout<<"before sync: nloc="<<localFields_[Pstream::myProcNo()].size()<<", nglob="<<this->size()
      <<", avg="<<average(localFields_[Pstream::myProcNo()])<<"/"<<average(*this)<<endl;
    
    reduce(*this, sumOp<Field<T> >());
    label ofs0=0;
    for (label j=0; j<Pstream::myProcNo(); j++)
      ofs0+=localSizes_[j];
    
    Field<T>& myField=localFields_[Pstream::myProcNo()];
    forAll(myField, i)
    {
      myField[i]=this->Field<T>::operator[](ofs0+i);
    }
    
    Pout<<"after sync: nloc="<<localFields_[Pstream::myProcNo()].size()<<", nglob="<<this->size()
      <<", avg="<<average(localFields_[Pstream::myProcNo()])<<"/"<<average(*this)<<endl;
  }
  
  void syncLocalToGlobal()
  {
    Pout<<"before sync: nloc="<<localFields_[Pstream::myProcNo()].size()<<", nglob="<<this->size()
      <<", avg="<<average(localFields_[Pstream::myProcNo()])<<"/"<<average(*this)<<endl;
    
    Pstream::gatherList(localFields_);
    Pstream::scatterList(localFields_);   
    
    this->Field<T>::operator=
    (
      ListListOps::combine<List<T> >
      (
	localFields_, accessOp<List<T> >()
      )
    );
	
    Pout<<"after sync: nloc="<<localFields_[Pstream::myProcNo()].size()<<", nglob="<<this->size()
      <<", avg="<<average(localFields_[Pstream::myProcNo()])<<"/"<<average(*this)<<endl;
  }
};


class globalPatch
: public PrimitivePatch<face, List, pointField>
{
  
  labelList procOfs_;
  label nfaces_;

  template
  <
      class Face,
      template<class> class FaceList,
      class PointField,
      class PointType=point
  >
  autoPtr<PrimitivePatch<face, List, pointField> > createGlobalPatch(
          const PrimitivePatch<Face, FaceList, PointField, PointType>& patch,
          const polyMesh* mesh
          )
  {
      autoPtr<PrimitivePatch<face, List, pointField> > result;

      if (mesh)
      {

          IOobject addrHeader
                  (
                      "pointProcAddressing",
                      mesh->facesInstance()/mesh->meshSubDir,
                      *mesh,
                      IOobject::MUST_READ
                      );

          if (UNIOF_HEADEROK(addrHeader, labelIOList))
          {
              // There is a pointProcAddressing file so use it to get labels
              // on the original mesh
              labelIOList gpi(addrHeader);

              typedef Tuple2<point,label> piTuple;
              typedef HashTable<piTuple,label,Foam::Hash<label> > pointHashTable;

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

              result.reset
                      (
                          new PrimitivePatch<face, List, pointField>(globalFaces, globalPoints)
                          );

              return result;
          }
      }

      result.reset
              (
                  new PrimitivePatch<face, List, pointField>(patch.localFaces(), patch.localPoints())
                  );
      return result;
  }

public:

  template
  <
      class Face,
      template<class> class FaceList,
      class PointField,
      class PointType=point
  >
  globalPatch(
          const PrimitivePatch<Face, FaceList, PointField, PointType>& patch,
          const polyMesh* mesh )
  : PrimitivePatch<face, List, pointField>(createGlobalPatch(patch, mesh)()),
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
  
  inline label toGlobalFaceI(const label& localFaceI) const
  {
    return procOfs_[Pstream::myProcNo()] + localFaceI;
  }
  
  template<class T>
  tmp< Field<T> > extractLocalFaceValues(const Field<T>& gfield) const
  {
    label pi=Pstream::myProcNo();
    label startI=procOfs_[pi];
    label endI=gfield.size()-1;
    if (pi<Pstream::nProcs()-1)
      endI=procOfs_[pi+1]-1;
    
    tmp<Field<T> > tres(new Field<T>(endI-startI+1));
    Field<T>& res=UNIOF_TMP_NONCONST(tres);
    
    label j=0;
    for(label i=startI; i<=endI; i++)
    {
      res[j++]=gfield[i];
    }
    
    return tres;
  }
  
  template<class T>
  void insertLocalFaceValues(const Field<T>& lfield, Field<T>& gfield) const
  {
    label pi=Pstream::myProcNo();
    
    label startI=procOfs_[pi];
    label endI=nfaces_-1;
    if (pi<Pstream::nProcs()-1)
      endI=procOfs_[pi+1]-1;
    
    label j=0;
    for(label i=startI; i<=endI; i++)
    {
      gfield[i]=lfield[j++];
    }
  }
  
};


}
#endif
