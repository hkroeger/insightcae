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
#include "../../../../../../../OpenFOAM/OpenFOAM-1.6-ext/src/OpenFOAM/db/IOstreams/IOstreams/Ostream.H"
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
    active_(true)
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

    const fvMesh& mesh=static_cast<const fvMesh&>(obr_);
    searchEngine_.reset(new meshSearch(mesh));
    
    read(dict);
    
  totalTime_= obr_.time().deltaTValue();

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
      totalTime_ = readScalar(propsDict.subDict(name_).lookup("totalTime"));
      correlationCoeffs_.reset(new tensorField(propsDict.subDict(name_).lookup("correlationCoeffs")));
      return;
    }
  }
  
  Info<< "    Starting averaging twoPointCorrelation at time " << obr_.time().timeName()
      << nl;
  
}


// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::twoPointCorrelation::~twoPointCorrelation()
{}

void Foam::twoPointCorrelation::read(const dictionary& dict)
{
  Info<<"read!"<<endl;
  if (active_)
    {
        const fvMesh& mesh=static_cast<const fvMesh&>(obr_);
	
	p0_ = point(dict.lookup("p0"));
	directionSpan_=vector(dict.lookup("directionSpan"));
	np_=readLabel(dict.lookup("np"));
    
	homogeneousTranslationSpan_=vector(dict.lookup("homogeneousTranslationSpan"));
	nph_=readLabel(dict.lookup("nph"));
    
	dictionary csysDict(dict.subDict("csys"));
	csys_=coordinateSystem::New
	(
	  word(csysDict.lookup("type")),
	  csysDict
	);
  
        Info<<"twoPointCorrelation "<<name_<<":"<<nl
            <<"    from point "<<p0_<<nl
            <<"    on "<<np_<<" points along "<<directionSpan_<<nl
            <<"    averaged over "<<nph_<<" copies along "<<homogeneousTranslationSpan_<<endl;
	    
	createInterpolators();
     }
}

void Foam::twoPointCorrelation::start()
{
  Info<<"start!"<<endl;
}

void Foam::twoPointCorrelation::execute()
{
  if (active_)
  {
  Info<<"execute!"<<endl;
  const volVectorField& U = obr_.lookupObject<volVectorField>("U");
  
  autoPtr<interpolation<vector> > interpolator
    (
        interpolation<vector>::New("cellPointFace", U)
    );
    
    
  tensorField cCoeffs(correlationCoeffs_().size(), tensor::zero);
  forAll(lines_, i)
  {
    const cloudSet& samples = lines_[i];

    vectorField values(np_, vector::zero); // Fixed size according to input params!
    
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
	    values[sampleI] = csys_().localVector
	    (
	      interpolator().interpolate
	      (
		  samplePt,
		  cellI,
		  faceI
	      )	      
	    );
	}
    }
    
    Pstream::listCombineGather(values, plusEqOp<vector>());
    Pstream::listCombineScatter(values);

    forAll(values, j)
    {
      cCoeffs[j] += values[0] * values[j]; //cmptMultiply(values[0], values[j]);
    }    
  }
  
  // averaging over homogeneous directions
  scalar dt = obr_.time().deltaTValue();
  scalar Dt = totalTime_;
  scalar alpha = (Dt - dt)/Dt;
  scalar beta = dt/Dt;

  correlationCoeffs_() = alpha * correlationCoeffs_() + beta*(cCoeffs/scalar(lines_.size()));
  
  totalTime_ += dt;
  
  if (obr_.time().outputTime())
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
    propsDict.add(name_, dictionary());
    propsDict.subDict(name_).add("totalTime", totalTime_);
    propsDict.subDict(name_).add("correlationCoeffs", correlationCoeffs_());
  }
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
                outputDir = obr_.time().path()/".."/"postProcessing"/name_/startTimeName;
            }
            else
            {
                outputDir = obr_.time().path()/"postProcessing"/name_/startTimeName;
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
  Info<<"write!"<<endl;
  if (active_)
    {
      makeFile();
      
      if (Pstream::master())
      {
	filePtr_()<<obr_.time().value()<<token::TAB;
	for (label k=0; k < pTraits<tensor>::nComponents; k++)
	{
	  for (label i=0; i<np_; i++)
	  {
	    filePtr_()<<correlationCoeffs_()[i][k]<<token::SPACE;
	  }
	  filePtr_()<<token::TAB;
	}
	filePtr_()<<endl;
      }
    }
}

void Foam::twoPointCorrelation::createInterpolators()
{
    Info<<"createIpol!"<<endl;
  const fvMesh& mesh=static_cast<const fvMesh&>(obr_);

  lines_.resize(nph_);
  for (label i=0; i<nph_; i++)
  {
    List<point> pts(np_);
    for (label j=0; j<np_; j++)
      pts[j]=csys_().globalPosition
      ( 
	p0_ 
	 + scalar(i)/scalar(nph_-1) * homogeneousTranslationSpan_
	 + scalar(j)/scalar(np_-1)  * directionSpan_ 
      );
    
    lines_.set
    (
      i,
      new cloudSet
      (
	name_,
	mesh,
	searchEngine_(),
	"distance",
	pts
      )
    );
  }
  
  bool reset=false;
  if (!correlationCoeffs_.valid()) 
    reset=true;
  else
    if (correlationCoeffs_().size()!=np_) reset=true;
  
  if (reset) 
  {
    Info << "Reset averaging because parameters became incompatible to previous averaging."<<endl;
    filePtr_.reset();
    correlationCoeffs_.reset(new tensorField(np_, tensor::zero));
  }
  
}

void Foam::twoPointCorrelation::updateMesh(const mapPolyMesh&)
{
    createInterpolators();
}


void Foam::twoPointCorrelation::movePoints(const polyMesh&)
{
    createInterpolators();
}


void Foam::twoPointCorrelation::readUpdate(const polyMesh::readUpdateState state)
{
    if (state != polyMesh::UNCHANGED)
    {
        createInterpolators();
    }
}

bool Foam::twoPointCorrelation::timeSet()
{
  // Do nothing - only valid on write
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

namespace Foam
{
    defineNamedTemplateTypeNameAndDebug(twoPointCorrelationFunctionObject, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        twoPointCorrelationFunctionObject,
        dictionary
    );
}


// ************************************************************************* //
