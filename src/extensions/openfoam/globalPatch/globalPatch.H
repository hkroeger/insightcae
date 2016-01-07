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
  List<Field<T> > localFields_;
  
public:
  
  GlobalField()
  : localFields_(Pstream::nProcs())
  {}
  
  GlobalField(label globalsize, label localsize)
  : localFields_(Pstream::nProcs())
  {
    resize(globalsize, localsize);
  }
  
  GlobalField(const Field<T>& localfield)
  : Field<T>(globalFieldSize<T>(localfield), pTraits<T>::zero),
    localFields_(Pstream::nProcs())
  {
    insertLocalValues(localfield);
  }
  
  void resize(label globalsize, label localsize)
  {
    Field<T>::resize(globalsize);
    localFields_[Pstream::myProcNo()].resize(localsize);
  }
  
  const Field<T>& getLocalValues() const
  {
    sync();
    return localFields_[Pstream::myProcNo()];
  }

  void setLocalValue(label localI, const T& value)
  {
    localFields_[Pstream::myProcNo()][localI]=value;
  }

  void insertLocalValues(const Field<T>& localfield)
  {
    localFields_[Pstream::myProcNo()]=localfield;
    sync();
  }
  
  void sync()
  {
    Pstream::gatherList(localFields_);
    Pstream::scatterList(localFields_);    
  }
};


class globalPatch
: public PrimitivePatch<face, List, pointField>
{
  
  labelList procOfs_;
  label nfaces_;
  
  autoPtr< PrimitivePatch<face, List, pointField> > createGlobalPatch(const polyPatch& patch);

public:
  globalPatch(const polyPatch& patch);
  
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
    Field<T>& res=tres();
    
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
