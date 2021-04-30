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
#include "fixedGradientFvPatchFields.H"
#include "SRFModel.H"
#include "fvMeshTools.H"
#include "repatchPolyTopoChanger.H"
#include "topoSet.H"
#include "processorMeshes.H"


#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
    argList::validArgs.append("new patch positions");
    argList::validOptions.insert("overwrite", "");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

    const word oldInstance = mesh.pointsInstance();

    bool overwrite=UNIOF_OPTIONFOUND(args, "overwrite");
    HashTable<label, word> newPos( IStringStream( UNIOF_ADDARG(args, 0) )() );

    DynamicList<word> unspecified;
    Map<word> specified;

    label nPatches=mesh.boundary().size();
    label nBoundaryFaces=0;
    forAll (mesh.boundary(), pI)
    {
      auto& bp=mesh.boundary()[pI];
      unspecified.append( bp.name() );
      nBoundaryFaces+=bp.size();
    }
    for(auto j=newPos.begin(); j!=newPos.end(); ++j)
    {
      if (j()>nPatches-1)
      {
        FatalErrorIn("main.cpp") << "The specified new position of patch \""<<j.key()<<"\" is beyond the end of the patch list!"<<abort(FatalError);
      }
      if (!unspecified.found(j.key()))
      {
        FatalErrorIn("main.cpp") << "There is either no patch with name \""<<j.key()<<"\" or its new index was specified multiple times!"<<abort(FatalError);
      }
      else
      {
        unspecified.remove(unspecified.find(j.key()));
        if (specified.found(j()))
        {
          FatalErrorIn("main.cpp") << "The new index "<<j()<<" was specified multiple times!"<<abort(FatalError);
        }
        specified.insert(j(), j.key());
      }
    }
    unspecified.shrink();


    labelList oldToNew(nPatches, -1);
    int u=0;
    for (label i=0; i<nPatches; ++i)
    {
      word curPatchName;
      if (specified.found(i))
      {
        curPatchName=specified[i];
      }
      else
      {
        curPatchName=unspecified[u++];
      }
      Info<<"Moving patch \""<<curPatchName<<" to index "<<i<<endl;
      oldToNew[mesh.boundary().findPatchID(curPatchName)]=i;
    }

    Info<<"Reordering patches..."<<endl;

    labelListList newPatchID(nBoundaryFaces, {-1,-1});
    label i=0;
    forAll (mesh.boundary(), pI)
    {
      auto& bp=mesh.boundary()[pI];
      forAll(bp, j)
      {
        label fI=bp.start()+j;
        //newPatchID[fI]=oldToNew[pI];
        newPatchID[i++]={fI,oldToNew[pI]};
      }
    }

    List<polyPatch*> newPatchPtrList(nPatches);
    for (label pI = 0; pI < nPatches; pI++)
    {
      label newPatchi=oldToNew[pI];
      const polyPatch& bp=mesh.boundary()[pI].patch();
      const polyPatch& obp=mesh.boundary()[newPatchi].patch();

      newPatchPtrList[newPatchi] =
           bp.clone(
             mesh.boundaryMesh(),
             newPatchi,
             obp.size(), obp.start() // use startFace and size of old patch at this place in the list
          ).ptr();
    }

    repatchPolyTopoChanger polyMeshRepatcher(mesh);

    polyMeshRepatcher.changePatches(newPatchPtrList);

    forAll(newPatchID, i)
    {
      const auto& fI_npI=newPatchID[i];
      polyMeshRepatcher.changePatchID(fI_npI[0], fI_npI[1]);
    }

    polyMeshRepatcher.repatch();

    if (!overwrite)
    {
      runTime++;
      mesh.setInstance(runTime.timeName());
    }
    else
    {
      mesh.setInstance(oldInstance); // no output of mesh.write() without
    }

    Info<< "Writing mesh to " << runTime.timeName() << endl;
    mesh.write();
    topoSet::removeFiles(mesh);
    processorMeshes::removeFiles(mesh);

//    forAll(mesh.boundary(), pI)
//        Info<<pI<<": "<<mesh.boundary()[pI].name()<<endl;

    Info<<"End\n"<<endl;

    return 0;
}
