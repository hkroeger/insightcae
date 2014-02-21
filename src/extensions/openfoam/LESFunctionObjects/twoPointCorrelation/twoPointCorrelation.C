/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "twoPointCorrelation.H"
#include "dictionary.H"
#include "dlLibraryTable.H"
#include "Time.H"
#include "interpolation.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
  defineTypeNameAndDebug(twoPointCorrelation, 0);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::twoPointCorrelation::twoPointCorrelation
(
  const word& name,
    const objectRegistry& obr,
    const dictionary& dict,
    const bool loadFromFiles
)
:
    name_(name),
    obr_(obr),
    active_(true),
    searchEngine_(mesh_)
{
      // Check if the available mesh is an fvMesh, otherwise deactivate
    if (!isA<fvMesh>(obr_))
    {
        active_ = false;
        WarningIn
        (
            "minMaxSurfacePressure::minMaxSurfacePressure"
            "(const objectRegistry&, const dictionary&)"
        )   << "No fvMesh available, deactivating." << nl
            << endl;
    }

    read(dict);
}


// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::twoPointCorrelation::~twoPointCorrelation()
{}

void Foam::twoPointCorrelation::read(const dictionary& dict)
{
    if (active_)
    {
        const fvMesh& mesh=static_cast<const fvMesh&>(obr_);

	point p0(dict.lookup("p0"));
	point p1(dict.lookup("p1"));
	label np=readLabel(dict.lookup("np"));
	
        Info<<"twoPointCorrelation "<<name_<<":"<<nl
            <<"    p0="<<p0<<nl
            <<"    p1="<<p1<<endl;
	    
	lines_.resize(1);
	lines_.set(0,
	  new uniformSet
	  (
	    name_,
	    mesh,
            searchEngine_,
            "distance",
            p0, p1, np
	  )
	 );
	
	correlationCoeffs_.reset(new tensorField(np, tensor::zero));
     }
}

void Foam::twoPointCorrelation::start()
{
  totalTime_= db_.time().deltaTValue();

  IOobject propsDictHeader
  (
      "twoPointCorrelationProperties",
      obr_.time().timeName(obr_.time().startTime().value()),
      "uniform",
      obr_,
      IOobject::MUST_READ_IF_MODIFIED,
      IOobject::NO_WRITE,
      false
  );

  if (propsDictHeader.headerOk())
  {
    IOdictionary propsDict(propsDictHeader);
    
    if (propsDict.found(name_))
    {
      Info<< "    Restarting averaging for twoPointCorrelation site " << name_ << nl;
      totalTime_=readScalar(propsDict.lookup(name_));
      return;
    }
  }
  
  Info<< "    Starting averaging twoPointCorrelation at time " << obr_.time().timeName()
      << nl;
}

void Foam::twoPointCorrelation::execute()
{
  const volVectorField& U = db_.lookupObject<volVectorField>("U");
  
  autoPtr<interpolation<vector> > interpolator
    (
        interpolation<vector>::New("cellPointFace", U)
    );
    
    
  tensorField cCoeffs(correlationCoeffs_.size(), tensor::zero);
  forAll(lines_, i)
  {
    const uniformSet& samples = lines_[i];

    vectorField values(samples.size(), vector::zero);
    
    forAll(samples, sampleI)
    {
	const point& samplePt = samples[sampleI];
	label cellI = samples.cells()[sampleI];
	label faceI = samples.faces()[sampleI];

	if (cellI == -1 && faceI == -1)
	{
	    // Special condition for illegal sampling points
	    values[sampleI] = pTraits<vector>::zero;
	}
	else
	{
	    values[sampleI] = interpolator().interpolate
	    (
		samplePt,
		cellI,
		faceI
	    );
	}
    }
    
    Pstream::listCombineGather(values, plusEqOp<vectorField>());
    Pstream::listCombineScatter(values);

    forAll(values, j)
    {
      cCoeffs[j]+=cmptMultiply(values[0], values[j]);
    }    
  }
  
  // averaging over homogeneous directions
  scalar dt = db_.time().deltaTValue();
  scalar Dt = totalTime_;
  scalar alpha = (Dt - dt)/Dt;
  scalar beta = dt/Dt;

  correlationCoeffs_ = alpha * correlationCoeffs_ + beta*(cCoeffs/scalar(lines_.size()));
  
  totalTime_ += dt;
  
  if (db_.time().outputTime())
  {
    IOdictionary propsDict
    (
        IOobject
        (
            "twoPointCorrelationProperties",
            obr_.time().timeName(),
            "uniform",
            obr_,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
        )
    );
    propsDict.add(name_, totalTime_);
  }
}


void Foam::twoPointCorrelation::end()
{
    // Do nothing - only valid on write
}

void Foam::twoPointCorrelation::makeFile()
{
    // Create the minmax file if not already created
    if (filePtr_.empty())
    {
        if (debug)
        {
            Info<< "Creating twoPointCorrelation file." << endl;
        }

        // File update
        if (Pstream::master())
        {
            fileName outputDir;
            word startTimeName =
                obr_.time().timeName(obr_.time().startTime().value());

            if (Pstream::parRun())
            {
                // Put in undecomposed case (Note: gives problems for
                // distributed data running)
                outputDir = obr_.time().path()/".."/name_/startTimeName;
            }
            else
            {
                outputDir = obr_.time().path()/name_/startTimeName;
            }

            // Create directory if does not exist.
            mkDir(outputDir);

            // Open new file at start up
            filePtr_.reset(new OFstream(outputDir/(type() + ".dat")));

            // Add headers to output data
            writeFileHeader();
        }
    }
}

void Foam::twoPointCorrelation::writeFileHeader()
{
    if (filePtr_.valid())
    {
        filePtr_()
            << "# Time" << tab
            << "correlation values"
            << endl;
    }
}

void Foam::twoPointCorrelation::write()
{
    if (active_)
    {
      makeFile();
    }
}

void Foam::twoPointCorrelation::correct()
{
  // need to rebuild uniform sets
  FatalErrorIn("twoPointCorrelation::correct()")
  << "not implemented"<<abort(FatalError);
}

void Foam::twoPointCorrelation::updateMesh(const mapPolyMesh&)
{
    correct();
}


void Foam::twoPointCorrelation::movePoints(const polyMesh&)
{
    correct();
}


void Foam::twoPointCorrelation::readUpdate(const polyMesh::readUpdateState state)
{
    if (state != polyMesh::UNCHANGED)
    {
        correct();
    }
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //



// ************************************************************************* //
