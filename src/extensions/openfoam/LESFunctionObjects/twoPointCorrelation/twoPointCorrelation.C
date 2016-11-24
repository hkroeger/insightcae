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

#include "twoPointCorrelation.H"
#include "OFstream.H"
#include "dictionary.H"
#include "dlLibraryTable.H"
#ifdef Fx40
#include "foamTime.H"
#else
#include "Time.H"
#endif
#include "interpolation.H"

#include "SortableList.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
  defineTypeNameAndDebug(twoPointCorrelation, 0);
}

using namespace Foam;

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

    p0_(point::zero),
    directionSpan_(vector::zero),
    np_(0),
    homogeneousTranslationUnit_(vector::zero),
    nph_(0),
    totalTime_(0.0)
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

  if (active_)
  {
      const fvMesh& mesh=static_cast<const fvMesh&>(obr_);
      searchEngine_.reset(new meshSearch(mesh));

      read(dict);

      IOobject propsDictHeader
      (
	  "twoPointCorrelationProperties",
	  obr_.time().timeName(obr_.time().startTime().value()),
	  "uniform",
	  obr_,
	  IOobject::MUST_READ,
	  IOobject::NO_WRITE,
	  false
      );
      
      if (
#if defined(OFplus)
	propsDictHeader.typeHeaderOk<IOdictionary>()
#else
	propsDictHeader.headerOk()
#endif
      )
      {
	  IOdictionary propsDict(propsDictHeader);
	  
	  if (Pstream::master())
	  {
	      totalTime_= obr_.time().deltaTValue();

	      if (propsDict.found(name_))
	      {
		  Info<< "    Restarting averaging for twoPointCorrelation site " << name_ << nl;
		  totalTime_ = readScalar(propsDict.subDict(name_).lookup("totalTime"));
		  //propsDict.subDict(name_).lookup("correlationCoeffs") >> correlationCoeffs_();
		  for(label i=0; i<pTraits<tensor>::nComponents; i++)
		  {
		    Istream& is=propsDict.subDict(name_).lookup("correlationCoeffs");
		    label num=readLabel(is); // overread list size
		    if (num!=np_)
		      FatalErrorIn("read") << "number of sampling points does not match" <<abort(FatalError);
		    correlationCoeffs_.reset(new tensorField(readList<tensor>(is)));
		  }
		  Pout<<correlationCoeffs_()<<endl;
	      }
	  }
      }
  }
}

Foam::twoPointCorrelation::~twoPointCorrelation()
{}


void Foam::twoPointCorrelation::read(const dictionary& dict)
{
    if (active_)
    {
        p0_ = point(dict.lookup("p0"));
        directionSpan_=vector(dict.lookup("directionSpan"));
        np_=readLabel(dict.lookup("np"));

        homogeneousTranslationUnit_=vector(dict.lookup("homogeneousTranslationUnit"));
        nph_=readLabel(dict.lookup("nph"));

        dictionary csysDict(dict.subDict("csys"));
        csys_=coordinateSystem::New
              (
#if defined(OF23x) or defined(OFplus)
		  obr_,
#else
                  word(csysDict.lookup("type")),
#endif
                  csysDict
              );

        Info<<"Definition of twoPointCorrelation "<<name_<<":"<<nl
            <<"    from point "<<p0_<<" on "<<np_<<" points along "<<directionSpan_<<nl
            <<"    averaged over "<<nph_<<" copies, translated by "<<homogeneousTranslationUnit_<<endl;
	    
	createInterpolators();
    }
}

void Foam::twoPointCorrelation::start()
{
}


template<class T>
void combineSampledValues
(
    const volFieldSampler<T>& sampledField,
    const labelListList& indexSets,
    autoPtr<volFieldSampler<T> >& masterField
)
{
    List<Field<T> > masterValues(indexSets.size());

    forAll(indexSets, setI)
    {
        // Collect data from all processors
        List<Field<T> > gatheredData(Pstream::nProcs());
        gatheredData[Pstream::myProcNo()] = sampledField[setI];
        Pstream::gatherList(gatheredData);

        if (Pstream::master())
        {
            Field<T> allData
            (
                ListListOps::combine<Field<T> >
                (
                    gatheredData,
                    Foam::accessOp<Field<T> >()
                )
            );

            masterValues[setI] = UIndirectList<T>
                                 (
                                     allData,
                                     indexSets[setI]
                                 )();
        }
    }

    masterField.reset
    (
        new volFieldSampler<T>
        (
            masterValues,
            sampledField.name()
        )
    );
}

template<class Type>
autoPtr<volFieldSampler<Type> > sample
(
    PtrList<sampledSet>& sets,
    const GeometricField<Type, fvPatchField, volMesh>& field,
    const labelListList& indexSets
)
{
    // Storage for interpolated values
    volFieldSampler<Type> sampledField
    (
        "cellPointFace",
        field,
        sets
    );
    autoPtr<volFieldSampler<Type> > masterField;
    combineSampledValues(sampledField, indexSets, masterField);

    return masterField;
}

void Foam::twoPointCorrelation::combineSampledSets
(
    PtrList<coordSet>& masterSampledSets,
    labelListList& indexSets
)
{
    // Combine sampleSets from processors. Sort by curveDist. Return
    // ordering in indexSets.
    // Note: only master results are valid

    masterSampledSets_.clear();
    masterSampledSets_.setSize(lines_.size());
    indexSets_.setSize(lines_.size());

    const PtrList<sampledSet>& sampledSets = lines_;

    forAll(sampledSets, setI)
    {
        const sampledSet& samplePts = sampledSets[setI];

        // Collect data from all processors
        List<List<point> > gatheredPts(Pstream::nProcs());
        gatheredPts[Pstream::myProcNo()] = samplePts;
        Pstream::gatherList(gatheredPts);

        List<labelList> gatheredSegments(Pstream::nProcs());
        gatheredSegments[Pstream::myProcNo()] = samplePts.segments();
        Pstream::gatherList(gatheredSegments);

        List<scalarList> gatheredDist(Pstream::nProcs());
        gatheredDist[Pstream::myProcNo()] = samplePts.curveDist();
        Pstream::gatherList(gatheredDist);


        // Combine processor lists into one big list.
        List<point> allPts
        (
            ListListOps::combine<List<point> >
            (
                gatheredPts, accessOp<List<point> >()
            )
        );
        labelList allSegments
        (
            ListListOps::combine<labelList>
            (
                gatheredSegments, accessOp<labelList>()
            )
        );
        scalarList allCurveDist
        (
            ListListOps::combine<scalarList>
            (
                gatheredDist, accessOp<scalarList>()
            )
        );


        if (Pstream::master() && allCurveDist.size() == 0)
        {
            WarningIn("sampledSets::combineSampledSets(..)")
                    << "Sample set " << samplePts.name()
                    << " has zero points." << endl;
        }

        // Sort curveDist and use to fill masterSamplePts
        SortableList<scalar> sortedDist(allCurveDist);
        indexSets[setI] = sortedDist.indices();

        masterSampledSets.set
        (
            setI,
            new coordSet
            (
                samplePts.name(),
                samplePts.axis(),
                List<point>(UIndirectList<point>(allPts, indexSets[setI]))
#ifndef OF16ext
		,
                allCurveDist
#endif
            )
        );
    }
}




void Foam::twoPointCorrelation::execute()
{
  if (debug) Pout<<"twoPointCorrelation::execute "<<name_<<endl;
  
  if (active_)
    {
      
	if (!obr_.foundObject<volVectorField>("UMean"))
	{
	  //resetAveraging();
	  WarningIn("execute")
	    << "No mean velocity field in registry, delaying until next timestep"<<endl;
	}
	else
	{
	  
	  const volVectorField& U = obr_.lookupObject<volVectorField>("U");
	  const volVectorField& Umean = obr_.lookupObject<volVectorField>("UMean");
	  volVectorField uPrime = U-Umean;

	  autoPtr<OFstream> dbgFile;
	  if (debug && Pstream::master())
	  {
	      dbgFile.reset(new OFstream("twoPointCorrelation_"+name_+".csv"));
	      dbgFile() << "X,Y,Z,Vx,Vy,Vz,Vr,Vtheta,Vz" <<nl;
	  }

	  combineSampledSets(masterSampledSets_, indexSets_);
	  autoPtr<volFieldSampler<vector> > vfs = sample(lines_, uPrime, indexSets_);

	  if (Pstream::master())
	  {
	      tensorField cCoeffs(correlationCoeffs_().size(), tensor::zero);
	      forAll(vfs(), i)
	      {
		  //const cloudSet& samples = lines_[i];
		  const vectorField& values=vfs()[i];

		  //vectorField values(np_, vector::zero); // Fixed size according to input params!
		  for(label j=0; j<np_; j++)
		  {
		      if (dbgFile.valid())
		      {
			  const point& pt = masterSampledSets_[i][j];
			  const vector& v= values[j]; // in local CS
			  const vector& lv= csys_().localVector(values[j]); // in local CS
			  Info<<j<<" "<<pt<<" "<<v<<endl;
			  dbgFile() << pt.x()<<","<<pt.y()<<","<<pt.z()<<","<<v.x()<<","<<v.y()<<","<<v.z()<<","<<lv.x()<<","<<lv.y()<<","<<lv.z()<<nl;
		      }
		      cCoeffs[j] += csys_().localVector(values[0]) * csys_().localVector(values[j]); //cmptMultiply(values[0], values[j]);
		  }
	      }

	      if (dbgFile.valid())
	      {
		  dbgFile.reset();
	      }

	      // averaging over homogeneous directions
	      scalar dt = obr_.time().deltaTValue();
	      totalTime_ += dt;
	      scalar Dt = totalTime_;
	      scalar alpha = (Dt - dt)/Dt;
	      scalar beta = dt/Dt;

	      correlationCoeffs_() = alpha * correlationCoeffs_() + beta*(cCoeffs/scalar(lines_.size()));

	  }

	  Pstream::scatter(totalTime_);
	  Pstream::scatter(correlationCoeffs_());

	  
	  if (obr_.time().outputTime())
	  {
	    Pout<<"output"<<endl;
	      IOdictionary propsDict
	      (
		  IOobject
		  (
		      "twoPointCorrelationProperties",
		      obr_.time().timeName(),
		      "uniform",
		      obr_,
		      IOobject::READ_IF_PRESENT,
		      IOobject::NO_WRITE,
		      false
		  )
	      );
	      
	      //if (Pstream::master())
	      {
		propsDict.add(name_, dictionary());	      
		
		propsDict.subDict(name_).add("totalTime", totalTime_);
		
		//propsDict.subDict(name_).add("correlationCoeffs", static_cast<const List<tensor>&>(correlationCoeffs_()));
		//for(label i=0; i<pTraits<tensor>::nComponents; i++)
		  propsDict.subDict(name_).add
		  (
		    "correlationCoeffs", 
		    correlationCoeffs_()
		  );
		propsDict.regIOobject::write();
	      }
	  }
	}
    }
    
  if (debug) Pout<<"twoPointCorrelation::execute ended "<<name_<<endl;
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
                obr_.time().timeName(obr_.time().value());

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
  if (debug) Pout<<"twoPointCorrelation::writeFileHeader "<<name_<<endl;
  
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
  
  if (debug) Pout<<"twoPointCorrelation::write "<<name_<<endl;
  
    if (active_ && correlationCoeffs_.valid() )
    {
        makeFile();

        if (Pstream::master())
        {
	    if (debug)
	    {
	      Pout<<"write correlationCoeffs="<<correlationCoeffs_()<<endl;
	      Pout<<"using x_="<<x_()<<endl;
	    }
	    
            filePtr_()<<obr_.time().value()<<token::TAB;
            for (label k=0; k < pTraits<tensor>::nComponents; k++)
            {
                for (label i=0; i<correlationCoeffs_().size(); i++)
                {
                    filePtr_()<<correlationCoeffs_()[i][k]<<token::SPACE;
                }
                filePtr_()<<token::TAB;
            }

            tensor L=tensor::zero;
	    for(label l=0; l<correlationCoeffs_().size()-1; l++)
	    {
	      L += 0.5*(correlationCoeffs_()[l]+correlationCoeffs_()[l+1]) * (x_()[l+1]-x_()[l]);
	    }
	    if ( mag(correlationCoeffs_()[0]) > SMALL )
	    {
	      L=cmptDivide( L, correlationCoeffs_()[0]);
	    }
            for (label k=0; k < pTraits<tensor>::nComponents; k++)
            {
                filePtr_()<<L[k]<<token::SPACE;
            }

            filePtr_()<<endl;
        }
    }
    
}

void Foam::twoPointCorrelation::createInterpolators()
{
    Info << "Building interpolators for twoPointCorrelation "<<name_  << endl;
    if (debug) Pout << "createInterpolators " << name_  << endl;
    
    const fvMesh& mesh=static_cast<const fvMesh&>(obr_);

    lines_.resize(nph_);
    x_.reset(new scalarField(np_, 0.0));
    
    for (label i=0; i<nph_; i++)
    {
        pointField pts(np_);
		
        for (label j=0; j<np_; j++)
        {
            pts[j]=csys_().globalPosition
                   (
                       p0_
                       + scalar(i) * homogeneousTranslationUnit_
                       + scalar(j)/scalar(np_-1)  * directionSpan_
                   );
	    if ((i==0)&&(j>0))
	      x_()[j] = x_()[j-1] + mag(pts[j]-pts[j-1]);
        }

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

    combineSampledSets(masterSampledSets_, indexSets_);

    bool reset=false;
    if (!correlationCoeffs_.valid())
    {
      reset=true;
    }
    else if (correlationCoeffs_().size()!=np_) 
    {
      Info << "Reset averaging because parameters became incompatible to previous averaging."<<endl;
      reset=true;
    }

    if (reset) 
    {
      resetAveraging();
    }

}

void Foam::twoPointCorrelation::resetAveraging()
{
  totalTime_=0.0;
  filePtr_.reset();
  correlationCoeffs_.reset(new tensorField(np_, tensor::zero));
}

void Foam::twoPointCorrelation::updateMesh(const mapPolyMesh&)
{
    if (debug) Pout<<"updateMesh"<<endl;
    createInterpolators();
}

void Foam::twoPointCorrelation::movePoints(
#if ! (defined(OF16ext) || defined(OF21x))
    const polyMesh& mesh
#else
    const pointField& points
#endif
)
{
    if (debug) Pout<<"movePoints"<<endl;
    createInterpolators();
}

void Foam::twoPointCorrelation::readUpdate(const polyMesh::readUpdateState state)
{
    if (debug) Pout<<"readUpdate"<<endl;
    if (state != polyMesh::UNCHANGED)
    {
        createInterpolators();
    }
}

void Foam::twoPointCorrelation::timeSet()
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
