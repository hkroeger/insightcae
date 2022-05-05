/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016 OpenFOAM Foundation
     \\/     M anipulation  | Copyright (C) 2016 OpenCFD Ltd.
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

#include "extendedRigidBodyMotion.H"
#include "addToRunTimeSelectionTable.H"
#include "polyMesh.H"
#include "pointPatchDist.H"
#include "pointConstraints.H"
#include "uniformDimensionedFields.H"
#include "forces.H"
#include "mathematicalConstants.H"
#include "fvMesh.H"
#include "fvCFD.H"
#include "wallFvPatch.H"

#include "boost/format.hpp"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{

defineTypeNameAndDebug(extendedRigidBodyMeshMotion, 0);

addToRunTimeSelectionTable
(
    motionSolver,
    extendedRigidBodyMeshMotion,
    dictionary
);

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

autoPtr<Foam::extendedRigidBodyMeshMotion::directThrustForce> Foam::extendedRigidBodyMeshMotion::directThrustForce::New(Istream& is)
{
    dictionary d(is);
    return autoPtr<Foam::extendedRigidBodyMeshMotion::directThrustForce>
    (
        new Foam::extendedRigidBodyMeshMotion::directThrustForce
        (
            point(d.lookup("PoA")),
            vector(d.lookup("direction")),
            readScalar(d.lookup("resistance_fraction"))
        )
    );
}

Foam::extendedRigidBodyMeshMotion::directThrustForce::directThrustForce(const point&p, const vector&d, scalar f)
: PoA_(p), direction_(d), resistance_fraction_(f)
{}

autoPtr<Foam::extendedRigidBodyMeshMotion::directThrustForce>
Foam::extendedRigidBodyMeshMotion::directThrustForce::clone() const
{
  return autoPtr<Foam::extendedRigidBodyMeshMotion::directThrustForce>(new directThrustForce(PoA_, direction_, resistance_fraction_));
}

Foam::extendedRigidBodyMeshMotion::bodyMesh::bodyMesh
(
    const polyMesh& mesh,
    const word& name,
    const label bodyID,
    const dictionary& dict
)
:
    name_(name),
    bodyID_(bodyID),
    patches_(wordReList(dict.lookup("patches"))),
    patchSet_(mesh.boundaryMesh().patchSet(patches_)),
    di_(readScalar(dict.lookup("innerDistance"))),
    do_(readScalar(dict.lookup("outerDistance"))),
    weight_
    (
        IOobject
        (
            name_ + ".motionScale",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
        ),
        pointMesh::New(mesh),
        dimensionedScalar("zero", dimless, 0.0)
    ),
    globalMotionDirection_(dict.lookupOrDefault<vector>("globalMotionDirection", vector(1.,0,0)))
{
    if (dict.found("directThrustForces"))
    {
        directThrustForces_=PtrList<directThrustForce>(dict.lookup("directThrustForces"));
    }
}


Foam::extendedRigidBodyMeshMotion::extendedRigidBodyMeshMotion
(
    const polyMesh& mesh,
    const IOdictionary& dict
)
:
    displacementMotionSolver(mesh, dict, typeName),
    model_
    (
#if OF_VERSION>=060000 //defined(OFesi1806)
        mesh.time(),
#endif
        coeffDict(),
        IOobject
        (
            "rigidBodyMotionState",
            mesh.time().timeName(),
            "uniform",
            mesh
        ).typeHeaderOk<IOdictionary>(true)
      ? IOdictionary
        (
            IOobject
            (
                "rigidBodyMotionState",
                mesh.time().timeName(),
                "uniform",
                mesh,
                IOobject::READ_IF_PRESENT,
                IOobject::NO_WRITE,
                false
            )
        )
      : coeffDict()
    ),
    test_(coeffDict().lookupOrDefault<Switch>("test", false)),
    rhoInf_(1.0),
    rhoName_(coeffDict().lookupOrDefault<word>("rho", "rho")),
    curTimeIndex_(-1),
    referenceSystemSpeed_(coeffDict().lookupOrDefault<vector>("referenceSystemSpeed", vector::zero)),
    rampDuration_(readScalar(coeffDict().lookup("rampDuration")))
{
    if (rhoName_ == "rhoInf")
    {
        rhoInf_ = readScalar(coeffDict().lookup("rhoInf"));
    }

    const dictionary& bodiesDict = coeffDict().subDict("bodies");

    forAllConstIter(IDLList<entry>, bodiesDict, iter)
    {
        const dictionary& bodyDict = iter().dict();

        if (bodyDict.found("patches"))
        {
            const label bodyID = model_.bodyID(iter().keyword());

            if (bodyID == -1)
            {
                FatalErrorInFunction
                    << "Body " << iter().keyword()
                    << " has been merged with another body"
                       " and cannot be assigned a set of patches"
                    << exit(FatalError);
            }

            bodyMeshes_.append
            (
                new bodyMesh
                (
                    mesh,
                    iter().keyword(),
                    bodyID,
                    bodyDict
                )
            );
        }
    }

    // Calculate scaling factor everywhere for each meshed body
    forAll(bodyMeshes_, bi)
    {
        const pointMesh& pMesh = pointMesh::New(mesh);

        pointPatchDist pDist(pMesh, bodyMeshes_[bi].patchSet_, points0());

        pointScalarField& scale = bodyMeshes_[bi].weight_;

        // Scaling: 1 up to di then linear down to 0 at do away from patches
        scale.primitiveFieldRef() =
            min
            (
                max
                (
                    (bodyMeshes_[bi].do_ - pDist.primitiveField())
                   /(bodyMeshes_[bi].do_ - bodyMeshes_[bi].di_),
                    scalar(0)
                ),
                scalar(1)
            );

        // Convert the scale function to a cosine
        scale.primitiveFieldRef() =
            min
            (
                max
                (
                    0.5
                  - 0.5
                   *cos(scale.primitiveField()
                   *Foam::constant::mathematical::pi),
                    scalar(0)
                ),
                scalar(1)
            );

        pointConstraints::New(pMesh).constrain(scale);
        //scale.write();
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::extendedRigidBodyMeshMotion::~extendedRigidBodyMeshMotion()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::pointField>
Foam::extendedRigidBodyMeshMotion::curPoints() const
{
    return points0() + pointDisplacement_.primitiveField();
}

tmp<pointField> Foam::extendedRigidBodyMeshMotion::curMotionStatePoints() const
{
    // Update the displacements
    if (bodyMeshes_.size() == 1)
    {
        pointDisplacement_.primitiveFieldRef() = model_.transformPoints
        (
            bodyMeshes_[0].bodyID_,
            bodyMeshes_[0].weight_,
            points0()
        ) - points0();
    }
    else
    {
        labelList bodyIDs(bodyMeshes_.size());
        List<const scalarField*> weights(bodyMeshes_.size());
        forAll(bodyIDs, bi)
        {
            bodyIDs[bi] = bodyMeshes_[bi].bodyID_;
            weights[bi] = &bodyMeshes_[bi].weight_;
        }

        pointDisplacement_.primitiveFieldRef() =
            model_.transformPoints(bodyIDs, weights, points0()) - points0();
    }

    // Displacement has changed. Update boundary conditions
    pointConstraints::New
    (
        pointDisplacement_.mesh()
    ).constrainDisplacement(pointDisplacement_);
    
    return points0() + pointDisplacement_.primitiveField();
}

const PtrList<extendedRigidBodyMeshMotion::bodyMesh> &extendedRigidBodyMeshMotion::bodyMeshes() const
{
    return bodyMeshes_;
}

const RBD::rigidBodyMotion& extendedRigidBodyMeshMotion::model() const
{
    return model_;
}

const RBD::rigidBodyModelState &extendedRigidBodyMeshMotion::motionState() const
{
    return model_.state();
}

bool extendedRigidBodyMeshMotion::isBodyPatch(label patchID) const
{
    forAll(bodyMeshes(), bi)
    {
        for (const label patchi : bodyMeshes()[bi].patchSet_)
        {
            if (patchID==patchi) return true;
        }
    }
    return false;
}
       
       
Foam::wordList filterPatchType(const Foam::fvMesh& mesh, const Foam::wordReList& patches, Foam::word filterTypeName, bool NOT=false)
{
    Foam::labelHashSet patchids = mesh.boundaryMesh().patchSet(patches);
    Foam::labelHashSet targ_patchids;
    
    forAllConstIter(Foam::labelHashSet, patchids, it)
    {
        if (mesh.boundary()[it.key()].type() == filterTypeName)
        {
            if (!NOT) targ_patchids.insert( it.key() );
        }
        else if (NOT) targ_patchids.insert( it.key() );
    }
    
    wordList result(targ_patchids.size());
    Foam::label i=0;
    forAllConstIter(Foam::labelHashSet, targ_patchids, it)
    {
        result[i++] = mesh.boundaryMesh()[it.key()].name();
    }    
    
    Info<<"filter patchlist for "<<filterTypeName<<" : "<<result<<endl;
    return result;
}


    
class patchMomentumForce
{
    vector F_, M_;
    
public:
    patchMomentumForce
    (
        const fvMesh& mesh, 
        const wordList& patchNames,
        tmp<volScalarField> rho,
        vector CofR = vector::zero
    )
    : F_(vector::zero), M_(vector::zero)
    {
        const volVectorField& U=mesh.lookupObject<volVectorField>("U");
        const volScalarField& p = mesh.lookupObject<volScalarField>("p");
        
        forAll(patchNames, i)
        {
            label id=mesh.boundaryMesh().findPatchID(patchNames[i]);
            
            const scalarField& rhop = rho().boundaryField()[id];
            const vectorField& Up = U.boundaryField()[id];
            const scalarField& pp = p.boundaryField()[id];
            const vectorField& Ap = mesh.Sf().boundaryField()[id];
            
            
            vectorField f(
              rhop * Up * (Up & Ap)
              +
              pp*Ap
              );
              
            vector F = gSum( f );
            vector M = gSum( mesh.Cf().boundaryField()[id] ^ f );
            
            Info << "[" << patchNames[i] << "] : F="<<F<<", M="<<M<<endl;
            
            F_ += F;
            M_ += M;
        }
    }
    
    vector force() const { return F_; }
    vector moment() const { return M_; }
};




void Foam::extendedRigidBodyMeshMotion::solve()
{
    const Time& t = mesh().time();

    scalar ramp=1.0;
    if (rampDuration_>0.)
    {
        double x=min(1.0, t.value()/rampDuration_);
//         ramp=pow(x, 2);
        ramp=pow((0.5*(::sin((x-0.5)*M_PI)+1.0)), 1);
        Info<<"Force ramping factor = "<<ramp<<endl;
    }

    if (mesh().nPoints() != points0().size())
    {
        FatalErrorInFunction
            << "The number of points in the mesh seems to have changed." << endl
            << "In constant/polyMesh there are " << points0().size()
            << " points; in the current mesh there are " << mesh().nPoints()
            << " points." << exit(FatalError);
    }
    

    // Store the motion state at the beginning of the time-step
    if (curTimeIndex_ != this->db().time().timeIndex())
    {
        // Create the output file if not already created
        if (filePtr_.empty())
        {
            if (debug)
            {
                Info<< "Creating output file." << endl;
            }

            // File update
            if (Pstream::master())
            {
                fileName outdir;
                
                word startTimeName =
                    mesh().time().timeName(mesh().time().startTime().value());

                if (Pstream::parRun())
                {
                    // Put in undecomposed case (Note: gives problems for
                    // distributed data running)
                    outdir = mesh().time().path()/".."/"postProcessing"/"rigidBodyMotion"/startTimeName;
                }
                else
                {
                    outdir = mesh().time().path()/"postProcessing"/"rigidBodyMotion"/startTimeName;
                }

                // Create directory if does not exist.
                mkDir(outdir);

                // Open new file at start up
                filePtr_.reset(new OFstream(outdir/"rigidBodyMotion.dat"));
                
                filePtr_() << "#time q qdot"<<endl;
            }
        }
        
        // write position and rotation
        if (Pstream::master())
        {
            filePtr_() << mesh().time().value();
            writeLogLine(filePtr_());
            filePtr_()<<endl;
        }
        
        model_.newTime();
        curTimeIndex_ = this->db().time().timeIndex();
    }

    const objectRegistry& gobr =
#if OF_VERSION<060505
            db()
#else
            t
#endif
            ;
    if (gobr.foundObject<uniformDimensionedVectorField>("g"))
    {
        model_.g() =
            ramp*gobr.lookupObject<uniformDimensionedVectorField>("g").value();
    } else
    {
        WarningIn("solve") << "no gravity defined!" << endl;
    }

    if (test_)
    {
        label nIter(readLabel(coeffDict().lookup("nIter")));

        for (label i=0; i<nIter; i++)
        {
            model_.solve
            (
#if OF_VERSION>=060000 //defined(OFesi1806)
                t.value(),
#endif
                t.deltaTValue(),
                scalarField(model_.nDoF(), Zero),
                Field<spatialVector>(model_.nBodies(), Zero)
            );
        }
    }
    else
    {
        Field<spatialVector> fx(model_.nBodies(), Zero);

        const fvMesh& fvmesh = dynamic_cast<const fvMesh&>(mesh());
        
        forAll(bodyMeshes_, bi)
        {
            const bodyMesh& body = bodyMeshes_[bi];
            const label bodyID = body.bodyID_;

            dictionary forcesDict;
            forcesDict.add("type", functionObjects::forces::typeName);
            forcesDict.add("patches", filterPatchType(fvmesh, bodyMeshes_[bi].patches_, wallFvPatch::typeName) );
            forcesDict.add("rhoInf", rhoInf_);
            forcesDict.add("rho", rhoName_);
            forcesDict.add("CofR", vector::zero);

            functionObjects::forces f("forces", db(), forcesDict);
            f.calcForcesMoment();

            Foam::tmp<Foam::volScalarField> rho;
            
            {
                if (rhoName_ == "rhoInf")
                {
                    rho=tmp<volScalarField>
                    (
                        new volScalarField
                        (
                            IOobject
                            (
                                "rho",
                                fvmesh.time().timeName(),
                                fvmesh
                            ),
                            fvmesh,
                            dimensionedScalar("rho", dimDensity, rhoInf_)
                        )
                    );
                }
                else
                {
                    rho=tmp<volScalarField>(new volScalarField(fvmesh.lookupObject<volScalarField>(rhoName_)));
                }
            }

            patchMomentumForce pmf
            (
                fvmesh,
                filterPatchType(fvmesh, bodyMeshes_[bi].patches_, wallFvPatch::typeName, true),
                rho/*,
                referenceSystemSpeed_*/
            );

            // calculate direct forces
            scalar R = f.forceEff() & body.globalMotionDirection_;
            vector dF=vector::zero, dM=vector::zero;
            forAll(body.directThrustForces_, j)
            {
                const directThrustForce& df = body.directThrustForces_[j];
                point cur_poa = model_.transformPoints(bodyID, pointField(1, df.PoA_))()[0];
                point cur_poa_ofs = model_.transformPoints(bodyID, pointField(1, df.PoA_+df.direction_))()[0];
                vector cur_dir = cur_poa_ofs - cur_poa; cur_dir/=mag(cur_dir);
                point cur_ctr = model_.transformPoints(bodyID, pointField(1, model_.bodies()[bodyID].c()))()[0];
                
                Info<<"PoA : "<<df.PoA_<<cur_poa<<" "<<cur_poa_ofs<<endl;
                Info<<"Dir : "<<cur_dir<<endl;
                Info<<"Ctr : "<<model_.bodies()[bodyID].c()<<cur_ctr<<endl;
                
                vector ddf = - (R * df.resistance_fraction_ / (df.direction_ & body.globalMotionDirection_)) * cur_dir; // scale force
                vector ddm = ( cur_poa - cur_ctr ) ^ ddf;
                dF+=ddf;
                dM+=ddm;
            }
            
            fx[bodyID] = spatialVector
            ( 
                ramp*(f.momentEff() + pmf.moment() + dM), 
                ramp*(f.forceEff() + pmf.force() + dF) 
            );
            
            // patch momentum force influence
            {
                vector mfrac=1e2*cmptDivide(pmf.moment(), f.momentEff());
                vector ffrac=1e2*cmptDivide(pmf.force(), f.forceEff());
                Info
                    <<boost::str(boost::format("[%01d]: PMF %10s%10s%10s%10s%10s%10s\n")
                    %bodyID%"Fx/Fxhull"%"Fy/Fyhull"%"Fz/Fzhull"%"Mx/Mxhull"%"My/Myhull"%"Mz/Mzhull").c_str()
                    <<boost::str(boost::format("[%01d]:      %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%%\n")
                    %bodyID%ffrac.x()%ffrac.y()%ffrac.z()%mfrac.x()%mfrac.y()%mfrac.z()).c_str()
                    ;
            }
            
            // direct force influence
            {
                vector mfrac=1e2*cmptDivide(dM, f.momentEff());
                vector ffrac=1e2*cmptDivide(dF, f.forceEff());
                Info
                    <<boost::str(boost::format("[%01d]:  DF %10s%10s%10s%10s%10s%10s\n")
                    %bodyID%"Fx/Fxhull"%"Fy/Fyhull"%"Fz/Fzhull"%"Mx/Mxhull"%"My/Myhull"%"Mz/Mzhull").c_str()
                    <<boost::str(boost::format("[%01d]:      %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%%\n")
                    %bodyID%ffrac.x()%ffrac.y()%ffrac.z()%mfrac.x()%mfrac.y()%mfrac.z()).c_str()
                    ;
            }
        }

        model_.solve
        (
#if OF_VERSION>=060000 //defined(OFesi1806)
            t.value(),
#endif
            t.deltaTValue(),
            scalarField(model_.nDoF(), Zero),
            fx
        );
    }

    if (Pstream::master() && model_.report())
    {
        forAll(bodyMeshes_, bi)
        {
            model_.status(bodyMeshes_[bi].bodyID_);
        }
    }

    // Update the displacements
    if (bodyMeshes_.size() == 1)
    {
        pointDisplacement_.primitiveFieldRef() = model_.transformPoints
        (
            bodyMeshes_[0].bodyID_,
            bodyMeshes_[0].weight_,
            points0()
        ) - points0();
    }
    else
    {
        labelList bodyIDs(bodyMeshes_.size());
        List<const scalarField*> weights(bodyMeshes_.size());
        forAll(bodyIDs, bi)
        {
            bodyIDs[bi] = bodyMeshes_[bi].bodyID_;
            weights[bi] = &bodyMeshes_[bi].weight_;
        }

        pointDisplacement_.primitiveFieldRef() =
            model_.transformPoints(bodyIDs, weights, points0()) - points0();
    }

    // Displacement has changed. Update boundary conditions
    pointConstraints::New
    (
        pointDisplacement_.mesh()
    ).constrainDisplacement(pointDisplacement_);
}


bool Foam::extendedRigidBodyMeshMotion::writeObject
(
#if OF_VERSION>=060505
            IOstreamOption streamOpt
#else
            IOstream::streamFormat fmt,
            IOstream::versionNumber ver,
            IOstream::compressionType cmp
#endif
#if OF_VERSION>=060000 //defined(OFesi1806)
    , const bool valid
#endif
) const
{
#if OF_VERSION>=060505
    streamOpt.format(IOstream::ASCII);
#endif

    IOdictionary dict
    (
        IOobject
        (
            "rigidBodyMotionState",
            mesh().time().timeName(),
            "uniform",
            mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
        )
    );

    model_.state().write(dict);

#if OF_VERSION>=060000 //defined(OFesi1806)
    // Force ascii writing
    return dict.regIOobject::writeObject(
#if OF_VERSION>=060505
                streamOpt,
#else
                IOstream::ASCII, ver, cmp,
#endif
                valid );
#else
    return dict.regIOobject::write();
#endif
}

void extendedRigidBodyMeshMotion::writeLogLine(Ostream &os) const
{
    forAll(model_.state().q(), i)
    {
        os<<token::SPACE << model_.state().q()[i];
    }
    forAll(model_.state().qDot(), i)
    {
        os<<token::SPACE << model_.state().qDot()[i];
    }
}


bool Foam::extendedRigidBodyMeshMotion::read()
{
    if (displacementMotionSolver::read())
    {
        model_.read(coeffDict());

        return true;
    }
    else
    {
        return false;
    }
}

}

// ************************************************************************* //
