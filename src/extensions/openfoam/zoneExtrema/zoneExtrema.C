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
#include "fixedGradientFvPatchFields.H"
#include "wallFvPatch.H"

#include "Tuple2.H"
#include "token.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
    timeSelector::addOptions();

    argList::validArgs.append("fieldName");
    argList::validArgs.append("cellZoneName");

#   include "setRootCase.H"
#   include "createTime.H"
    
    instantList timeDirs = timeSelector::select0(runTime, args);

#   include "createMesh.H"
//#   include "createFvOptions.H"
    
    word fieldName(IStringStream(
#if defined(OFplus)||defined(OFdev)
      args.arg(1)
#else
      args.additionalArgs()[0]
#endif
    )());
    
    word cellZoneName(IStringStream(
#if defined(OFplus)||defined(OFdev)
      args.arg(2)
#else
      args.additionalArgs()[1]
#endif
    )());
    

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        mesh.readUpdate();
	
	label cellZoneID = mesh.cellZones().findZoneID(cellZoneName);
	if (cellZoneID<0)
	{
	  FatalErrorIn("main")
	  << "cell zone "<<cellZoneName<<" not found."
	  << abort(FatalError);
	}
	const labelList& cells = mesh.cellZones()[cellZoneID];

	IOobject fieldheader
	(
	    fieldName,
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::NO_WRITE
	);
	
// 	if (!fieldheader.headerOk())
// 	  FatalErrorIn("main") << "Could not read field "<<fieldName<<abort(FatalError);	
// 	Info<<fieldheader.headerClassName()<<endl;

#if not defined(OFplus)	
	if (!fieldheader.headerOk())
#endif
	  
#if defined (OFplus)
	  if (fieldheader.typeHeaderOk<volVectorField>())
#else
	  if (fieldheader.headerClassName()=="volVectorField")
#endif
	  {
	    Info << "Reading vector field "<<fieldName<<"\n" << endl;
	    volVectorField field
	    (
	      fieldheader,
	      mesh
	    );	
	    
	    vectorField sub(cells.size());
	    forAll(cells, i)
	    {
	      sub[i]=field[cells[i]];
	    }
	    
	    vector mi=gMin(sub);
	    vector ma=gMax(sub);
	    Info<<"@t="<<runTime.timeName()<<" : min / max ["<<fieldName<<"]= "
	      <<mi.x()<<" "<<mi.y()<<" "<<mi.z()
	      <<" / "
	      <<ma.x()<<" "<<ma.y()<<" "<<ma.z()<<endl;
	  }
    }
}