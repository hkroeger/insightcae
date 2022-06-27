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
#include "cartesianCS.H"

#include "boost/format.hpp"

#include "uniof.h"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{


wordList filterPatchType(
        const fvMesh& mesh,
        const wordReList& patches,
        word filterTypeName,
        bool NOT=false )
{
    Foam::labelHashSet patchids =
            mesh.boundaryMesh().patchSet(patches);

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

//    Info<<"filter patchlist for "<<filterTypeName<<" : "<<result<<endl;
    return result;
}




PIController::PIController(double Kp, double TN)
    : Kp_(Kp), TN_(TN),
      integral_(0.)
{}

/**
 * @brief PIController
 * restore from previous writeState
 * @param is
 */
void PIController::restoreState(const dictionary& id)
{
    integral_ = id.lookupOrDefault<scalar>("integral", 0.);
    if (integral_>0.)
    {
        lastUpdate_.reset(
                  new Update{
                    readScalar(id.lookup("lastUpdateTime")),
                    readScalar(id.lookup("lastUpdateDeviation")),
                    readScalar(id.lookup("lastUpdateIntegral"))
                    } );
    }
}

scalar PIController::u(scalar t, scalar e)
{
    scalar I0=integral_;

    if ( lastUpdate_)
    {
        if ((t - lastUpdate_->t)<SMALL )
        {
            I0=lastUpdate_->integral;
        }
        integral_ = I0 + (t - lastUpdate_->t) * 0.5*(e + lastUpdate_->e);
    }


    lastUpdate_.reset(
                new Update{t, e, I0}
                );

    scalar u=Kp_* ( e + integral_/TN_ );
    Info << "@t="<<t<<", e="<<e<<", integral="<<integral_<<", u="<<u<<endl;

    return u;
}

void PIController::saveState(dictionary& od) const
{
    od.add("integral", integral_);
    if (lastUpdate_)
    {
        od.add("lastUpdateTime", lastUpdate_->t);
        od.add("lastUpdateDeviation", lastUpdate_->e);
        od.add("lastUpdateIntegral", lastUpdate_->integral);
    }
}



defineTypeNameAndDebug(extendedRigidBodyMeshMotion, 0);

addToRunTimeSelectionTable
(
    motionSolver,
    extendedRigidBodyMeshMotion,
    dictionary
);




// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //







extendedRigidBodyMeshMotion::bodyMesh::bodyMesh
(
    const polyMesh& mesh,
    const word& name,
    const label bodyID,
    const dictionary& dict,
    scalar rhoInf,
    const word& rhoName
)
:
    coordinateSystemSource(name),
    mesh_(mesh),
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
    globalMotionDirection_(
        dict.lookupOrDefault<vector>(
            "globalMotionDirection", vector(1.,0,0)))
{

    const fvMesh& fvmesh = dynamic_cast<const fvMesh&>(mesh);

    dictionary forcesDict;
    forcesDict.add("type",
                   extendedForces::typeName );
    forcesDict.add("patches",
                   filterPatchType(
                       fvmesh, patches_,
                       wallFvPatch::typeName ) );
    forcesDict.add("rhoInf", rhoInf);
    forcesDict.add("rho", rhoName);
    forcesDict.add("CofR", vector::zero);

    forcesFO_.reset(new extendedForces("body_"+name_, fvmesh, forcesDict));

    if (dict.found("directThrustForces"))
    {
        PtrList<extRBM::directForce> df(
                    dict.lookup("directThrustForces") );
        forAll(df, j)
        {
            additionalForces_.append(
                        autoPtr<extRBM::additionalForceAndMoment>(
                            df.set(j,nullptr) ) );
        }
    }

    auto pmfs=filterPatchType(
                fvmesh,
                patches_,
                wallFvPatch::typeName,
                true );

    forAll(pmfs, j)
    {
        additionalForces_.append(
                    new extRBM::patchMomentumForce
                    (
                        fvmesh,
                        pmfs[j],
                        rhoInf,
                        rhoName
                    )
                    );
    }
}

extendedForces &extendedRigidBodyMeshMotion::bodyMesh::forcesFO()
{
    return forcesFO_();
}

const extendedForces &extendedRigidBodyMeshMotion::bodyMesh::forcesFO() const
{
    return forcesFO_();
}

std::pair<vector,vector> extendedRigidBodyMeshMotion::bodyMesh::calcForcesMoments
(
        bool logForces
)
{
    forcesFO_().calcForcesMoment();

    vector pmF=vector::zero, pmM=vector::zero;
    forAll(additionalForces_, k)
    {
        const auto& af = additionalForces_[k];

        auto pFM=af.forceAndMoment();

        pmF+=pFM.first;
        pmM+=pFM.second;

        Info << af.type() << " [" << k << "]"
             << ": force = " << pFM.first << "(" << mag(pFM.first) << ")"
             << ", moment = " << pFM.second << "(" << mag(pFM.second) << ")"
             << endl;

        // force influence
        {
            vector mfrac=1e2*cmptDivide(pmM, forcesFO().momentEff());
            vector ffrac=1e2*cmptDivide(pmF, forcesFO().forceEff());
            Info
                << boost::str(boost::format(
                              "[%01d]: %3d %10s%10s%10s%10s%10s%10s\n" )
                             % bodyID_ % k
                             % "Fx/Fxhull" % "Fy/Fyhull" % "Fz/Fzhull"
                             % "Mx/Mxhull" % "My/Myhull" % "Mz/Mzhull" ).c_str()
                << boost::str(boost::format(
                              "[%01d]:      %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%%\n" )
                             % bodyID_
                             % ffrac.x() % ffrac.y() % ffrac.z()
                             % mfrac.x() % mfrac.y() % mfrac.z() ).c_str()
                ;
        }

        if (logForces && Pstream::master() )
        {
            auto& time = dynamic_cast<const fvMesh&>(mesh_).time();

            if (addForcesLog_.size()!=additionalForces_.size())
                addForcesLog_.resize(additionalForces_.size());

            if (!addForcesLog_(k))
            {
                autoPtr<OFstream> f;
                UNIOF_CREATEPOSTPROCFILE(
                            time,
                            "rigidBodyMotion",
                            str(boost::format("%s_%d")%af.type()%k),
                            f,
                            "# t Fx Fy Fz Mx My Mz" );
                addForcesLog_.set(k, f);
            }

            addForcesLog_[k]
                << time.value() << token::SPACE
                << pFM.first.x() << token::SPACE << pFM.first.y() << token::SPACE<< pFM.first.z() << token::SPACE
                << pFM.second.x() << token::SPACE << pFM.second.y() << token::SPACE<< pFM.second.z()
                << endl;
        }
    }


    return {forcesFO().forceEff() + pmF, forcesFO().momentEff() + pmM};
}



extendedRigidBodyMeshMotion::extendedRigidBodyMeshMotion
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
    referenceSystemSpeed_(
        coeffDict().lookupOrDefault<vector>(
            "referenceSystemSpeed", vector::zero ) ),
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
                    mesh, iter().keyword(),
                    bodyID, bodyDict,
                    rhoInf_, rhoName_
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

//        const fvMesh& fvmesh = dynamic_cast<const fvMesh&>(mesh);
//        auto pmfs=filterPatchType(
//                    fvmesh,
//                    bodyMeshes_[bi].patches_,
//                    wallFvPatch::typeName,
//                    true );

//        bodyMeshes_[bi].patchMomentumForces_.resize(pmfs.size());
//        forAll(pmfs, j)
//        {
//            bodyMeshes_[bi].patchMomentumForces_.set(
//                        j,
//                        new extRBM::patchMomentumForce
//                        (
//                            fvmesh,
//                            pmfs[j],
//                            rhoInf_,
//                            rhoName_
//                        )
//                        );
//        }
    }


    updateBodyMeshCoordinateSystems();
}




// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //




extendedRigidBodyMeshMotion::~extendedRigidBodyMeshMotion()
{}




// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //




tmp<pointField>
extendedRigidBodyMeshMotion::curPoints() const
{
    return points0() + pointDisplacement_.primitiveField();
}




tmp<pointField> extendedRigidBodyMeshMotion::curMotionStatePoints() const
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




PtrList<extendedRigidBodyMeshMotion::bodyMesh>&
extendedRigidBodyMeshMotion::bodyMeshes()
{
    return bodyMeshes_;
}




const PtrList<extendedRigidBodyMeshMotion::bodyMesh>&
extendedRigidBodyMeshMotion::bodyMeshes() const
{
    return bodyMeshes_;
}




const RBD::rigidBodyMotion&
extendedRigidBodyMeshMotion::model() const
{
    return model_;
}




const RBD::rigidBodyModelState&
extendedRigidBodyMeshMotion::motionState() const
{
    return model_.state();
}




bool extendedRigidBodyMeshMotion::isBodyPatch(label patchID) const
{
    forAll(bodyMeshes(), bi)
    {
        for (const label patchi: bodyMeshes()[bi].patchSet_)
        {
            if (patchID==patchi) return true;
        }
    }
    return false;
}
       











void extendedRigidBodyMeshMotion::solve()
{
    const Time& t = mesh().time();

    scalar ramp=1.0;
    if (rampDuration_>0.)
    {
        double x=min(1.0, t.value()/rampDuration_);
//         ramp=pow(x, 2);
        ramp=pow((0.5*(::sin((x-0.5)*M_PI)+1.0)), 1);
//        Info<<"Force ramping factor = "<<ramp<<endl;
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
        model_.newTime();
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
//        Field<spatialVector> fx(model_.nBodies(), Zero);

//        forAll(bodyMeshes_, bi)
//        {
//            const bodyMesh& body = bodyMeshes_[bi];
//            const label bodyID = body.bodyID_;

//            bodyMeshes_[bi].forcesFO().calcForcesMoment();

//            auto transformedPoint = [&](const vector& v) {
//                return model_.transformPoints(
//                            bodyID,
//                            pointField(1, v)
//                            )()[0];
//            };

//            point orgCtr=model_.bodies()[bodyID].c();
//            point curCtr = transformedPoint( orgCtr );

//            vector dF=vector::zero, dM=vector::zero;
//            if (directForcesLog_.size()!=body.directThrustForces_.size())
//                directForcesLog_.resize(body.directThrustForces_.size());
//            forAll(body.directThrustForces_, j)
//            {
//                const auto& df = body.directThrustForces_[j];

//                auto pf = df.PoAAndForce(transformedPoint);
//                vector ddf = pf.second;
//                vector ddm = ( pf.first/* - curCtr*/ ) ^ ddf;

//                Info << "Direct force " << j
//                     << ": force = " << ddf << "(" << mag(ddf) << ")"
//                     << ", moment = " << ddm << "(" << mag(ddm) << ")"
//                     << endl;

//                dF+=ddf;
//                dM+=ddm;

//                if (!directForcesLog_(j))
//                {
//                    autoPtr<OFstream> f;
//                    OStringStream logFName; logFName<<"directForce_"<<j;
//                    UNIOF_CREATEPOSTPROCFILE(
//                                mesh().time(),
//                                "rigidBodyMotion", logFName.str(),
//                                f);
//                    directForcesLog_.set(j, f);
//                }
//                directForcesLog_[j]
//                        << mesh().time().value() << token::SPACE
//                        << ddf.x() << token::SPACE << ddf.y() << token::SPACE << ddf.z() << token::SPACE
//                        << ddm.x() << token::SPACE << ddm.y() << token::SPACE << ddm.z()
//                        << endl;
//            }

//            vector pmF=vector::zero, pmM=vector::zero;
//            if (patchMomentumForcesLog_.size()!=body.patchMomentumForces_.size())
//                patchMomentumForcesLog_.resize(body.patchMomentumForces_.size());
//            forAll(body.patchMomentumForces_, k)
//            {
//                auto pFM=body.patchMomentumForces_[k].forceAndMoment();

//                Info << "Patch momentum force " << k
//                     << ": force = " << pFM.first << "(" << mag(pFM.first) << ")"
//                     << ", moment = " << pFM.second << "(" << mag(pFM.second) << ")"
//                     << endl;

//                pmF+=pFM.first;
//                pmM+=pFM.second;

//                if (!patchMomentumForcesLog_(k))
//                {
//                    autoPtr<OFstream> f;
//                    OStringStream logFName; logFName<<"patchMomentumForce_"<<k;
//                    UNIOF_CREATEPOSTPROCFILE(
//                                mesh().time(),
//                                "rigidBodyMotion", logFName.str(),
//                                f);
//                    patchMomentumForcesLog_.set(k, f);
//                }
//                patchMomentumForcesLog_[k]
//                        << mesh().time().value() << token::SPACE
//                        << pFM.first.x() << token::SPACE << pFM.first.y() << token::SPACE<< pFM.first.z() << token::SPACE
//                        << pFM.second.x() << token::SPACE << pFM.second.y() << token::SPACE<< pFM.second.z()
//                        << endl;
//            }
            
//            fx[bodyID] = spatialVector
//            (
//                ramp*(/*body.forcesFO().momentEff() + pmM + dM*/ FM.second ), // angular (moment)
//                ramp*(/*body.forcesFO().forceEff() + pmF + dF*/ FM.first ) // linear (force)
//            );
            
//            // patch momentum force influence
//            {
//                vector mfrac=1e2*cmptDivide(pmM, body.forcesFO().momentEff());
//                vector ffrac=1e2*cmptDivide(pmF, body.forcesFO().forceEff());
//                Info
//                    << boost::str(boost::format(
//                                  "[%01d]: PMF %10s%10s%10s%10s%10s%10s\n" )
//                                 % bodyID
//                                 % "Fx/Fxhull" % "Fy/Fyhull" % "Fz/Fzhull"
//                                 % "Mx/Mxhull" % "My/Myhull" % "Mz/Mzhull" ).c_str()
//                    << boost::str(boost::format(
//                                  "[%01d]:      %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%%\n" )
//                                 % bodyID
//                                 % ffrac.x() % ffrac.y() % ffrac.z()
//                                 % mfrac.x() % mfrac.y() % mfrac.z() ).c_str()
//                    ;
//            }
            
//            // direct force influence
//            {
//                vector mfrac=1e2*cmptDivide(dM, body.forcesFO().momentEff());
//                vector ffrac=1e2*cmptDivide(dF, body.forcesFO().forceEff());
//                Info
//                    << boost::str(boost::format(
//                                   "[%01d]:  DF %10s%10s%10s%10s%10s%10s\n" )
//                                  % bodyID
//                                  % "Fx/Fxhull" % "Fy/Fyhull" % "Fz/Fzhull"
//                                  % "Mx/Mxhull" % "My/Myhull" % "Mz/Mzhull" ).c_str()
//                    << boost::str(boost::format(
//                                   "[%01d]:      %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%%\n" )
//                                  % bodyID
//                                  % ffrac.x() % ffrac.y() % ffrac.z()
//                                  % mfrac.x() % mfrac.y() % mfrac.z() ).c_str()
//                    ;
//            }


        Field<spatialVector> fx(model_.nBodies(), Zero);

        forAll(bodyMeshes_, bi)
        {
            const bodyMesh& body = bodyMeshes_[bi];
            const label bodyID = body.bodyID_;

            auto FM = bodyMeshes_[bi].calcForcesMoments(
                        curTimeIndex_ != this->db().time().timeIndex()
                        );

            fx[bodyID] = spatialVector
            (
                ramp* FM.second, // angular (moment)
                ramp* FM.first  // linear (force)
            );
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

    updateBodyMeshCoordinateSystems();

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

    if (curTimeIndex_ != this->db().time().timeIndex())
    {
        continueLogFile();
    }

    curTimeIndex_ = this->db().time().timeIndex();
}


void extendedRigidBodyMeshMotion::updateBodyMeshCoordinateSystems()
{
    labelList bodyIDs(bodyMeshes_.size());
    forAll(bodyIDs, bi)
    {
        label bodyID = bodyMeshes_[bi].bodyID_;
        spatialTransform X(model_.X0(bodyID).inv() & model_.X00(bodyID));

        bodyMeshes_[bi].updateCoordinateSystem(
                    X.transformPoint(point(0,0,0)),
                    (X&spatialVector(vector::zero, vector(0,0,1))).l(),
                    (X&spatialVector(vector::zero, vector(1,0,0))).l()
                    );
    }
}


void extendedRigidBodyMeshMotion::bodyMesh::updateCoordinateSystem(
        const vector& p0, const vector& ez, const vector& ex )
{
    currentCoordinateSystem_.reset(
                new cartesianCS(
                    name_, p0,  ez, ex
                    )
                );

    Info<<"updated CS for body "<<name_<<": "
      <<currentCoordinateSystem_()<<endl;
}

autoPtr<coordinateSystem> extendedRigidBodyMeshMotion::bodyMesh::getCoordinateSystem() const
{
    return currentCoordinateSystem_->clone();
}

void extendedRigidBodyMeshMotion::populateRigidBodyMotionStateDict(
        dictionary& dict
        ) const
{
    model_.state().write(dict);
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

    populateRigidBodyMotionStateDict(dict);

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



void extendedRigidBodyMeshMotion::continueLogFile() const
{


    // Create the output file if not already created
    UNIOF_CREATEPOSTPROCFILE(
                mesh().time(),
                "rigidBodyMotion", "rigidBodyMotion.dat",
                rbmLogFile_,
                logFileHeaderLine() );

    // write position and rotation
    if (Pstream::master())
    {
        rbmLogFile_() << mesh().time().value();
        writeLogLine(rbmLogFile_());
        rbmLogFile_()<<endl;
    }
}

std::string extendedRigidBodyMeshMotion::logFileHeaderLine() const
{
    OStringStream header;

    header << "# t";
    forAll(model_.state().q(), i)
    {
        header << " q" << i;
    }
    forAll(model_.state().qDot(), i)
    {
        header << " qDot" << i;
    }

    return header.str();
}




void extendedRigidBodyMeshMotion::writeLogLine(
        Ostream &os
        ) const
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
