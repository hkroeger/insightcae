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




autoPtr<extendedRigidBodyMeshMotion::directThrustForce>
extendedRigidBodyMeshMotion::directThrustForce::New(Istream& is)
{
    dictionary d(is);
    autoPtr<Foam::extendedRigidBodyMeshMotion::directThrustForce> df
    (
        new Foam::extendedRigidBodyMeshMotion::directThrustForce
        (
            point(d.lookup("PoA")),
            vector(d.lookup("localDirection")),
            vector(d.lookup("verticalDirection")),
            forceSourceCombination(d.lookup("forceSource"))
        )
    );

    return df;
}




extendedRigidBodyMeshMotion::directThrustForce::directThrustForce(
        const point&p,
        const vector&d,
        const vector& v,
        const forceSourceCombination& fs
        )

: PoA_(p),
  localDirection_(d),
  verticalConstraint_(v),
  fs_(fs)
{}


std::pair<point,vector> extendedRigidBodyMeshMotion::directThrustForce::PoAAndForce
( std::function<point(point)> transformedPoint ) const
{
    point curPoA = transformedPoint( PoA_ );
    vector Forg=fs_.force(), F=Forg;

    Info<<"before trs F="<<Forg;
    scalar m=mag(localDirection_);
    if (m>SMALL)
    {
        vector curDir=transformedPoint( localDirection_/m + PoA_ ) - curPoA;
        curDir/=mag(curDir);

        F = curDir * (Forg & curDir);

        scalar mv=mag(verticalConstraint_);
        if (mv>SMALL)
        {
            vector eq = curDir^verticalConstraint_;
            vector el = verticalConstraint_^eq;
            el/=mag(el);

            scalar fplanereq=Forg&el;
            scalar fplanecur=F&el;

            F*=fplanereq/fplanecur;
        }
        Info<<", after trs F="<<F<<endl;
    }
    return {curPoA, F};
}




autoPtr<extendedRigidBodyMeshMotion::directThrustForce>
extendedRigidBodyMeshMotion::directThrustForce::clone() const
{
  return autoPtr<Foam::extendedRigidBodyMeshMotion::directThrustForce>(
              new directThrustForce(PoA_, localDirection_, verticalConstraint_, fs_) );
}




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
        directThrustForces_ =
                PtrList<directThrustForce>(
                    dict.lookup("directThrustForces") );
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
                    mesh,
                    iter().keyword(),
                    bodyID,
                    bodyDict,
                    rhoInf_,
                    rhoName_
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

        const fvMesh& fvmesh = dynamic_cast<const fvMesh&>(mesh);
        auto pmfs=filterPatchType(
                    fvmesh,
                    bodyMeshes_[bi].patches_,
                    wallFvPatch::typeName,
                    true );

        bodyMeshes_[bi].patchMomentumForces_.resize(pmfs.size());
        forAll(pmfs, j)
        {
            bodyMeshes_[bi].patchMomentumForces_.set(
                        j,
                        new patchMomentumForce
                        (
                            fvmesh,
                            pmfs[j],
                            rhoInf_,
                            rhoName_
                        )
                        );
        }
    }

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
       







extendedRigidBodyMeshMotion::patchMomentumForce::patchMomentumForce
(
    const fvMesh& mesh,
    const word& patchName,
    scalar rhoInf,
    const word &rhoName
)
: forceSource(patchName, true),
  mesh_(mesh),
  patchName_(patchName),
  rhoInf_(rhoInf),
  rhoName_(rhoName),
  F_(vector::zero), M_(vector::zero)
{}




void extendedRigidBodyMeshMotion::patchMomentumForce::calcFM()
{
    const volVectorField& U =
            mesh_.lookupObject<volVectorField>("U");
    const volScalarField& p =
            mesh_.lookupObject<volScalarField>("p");

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
                        mesh_.time().timeName(),
                        mesh_
                    ),
                    mesh_,
                    dimensionedScalar("rho", dimDensity, rhoInf_)
                )
            );
        }
        else
        {
            rho=tmp<volScalarField>(
                        new volScalarField(
                            mesh_.lookupObject<volScalarField>(
                                rhoName_) ) );
        }
    }

    label id=mesh_.boundaryMesh().findPatchID(patchName_);

    const scalarField& rhop = rho().boundaryField()[id];
    const vectorField& Up = U.boundaryField()[id];
    const scalarField& pp = p.boundaryField()[id];
    const vectorField& Ap = mesh_.Sf().boundaryField()[id];


    vectorField f(
      rhop * Up * (Up & Ap)
      +
      pp*Ap
      );

    F_ = gSum( f );
    M_ = gSum( mesh_.Cf().boundaryField()[id] ^ f );

    Info << "[" << patchName_ << "] : F="<<F_<<", M="<<M_<<endl;
}




vector extendedRigidBodyMeshMotion::patchMomentumForce::force() const
{
    const_cast<patchMomentumForce*>(this)->calcFM();
    return F_;
}




std::pair<vector,vector>
extendedRigidBodyMeshMotion::patchMomentumForce::forceAndMoment() const
{
    const_cast<patchMomentumForce*>(this)->calcFM();
    return {F_, M_};
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

//            dictionary forcesDict;
//            forcesDict.add("type",
//                           functionObjects::forces::typeName );
//            forcesDict.add("patches",
//                           filterPatchType(
//                               fvmesh,
//                               bodyMeshes_[bi].patches_,
//                               wallFvPatch::typeName ) );
//            forcesDict.add("rhoInf", rhoInf_);
//            forcesDict.add("rho", rhoName_);
//            forcesDict.add("CofR", vector::zero);

//            functionObjects::forces f("forces", db(), forcesDict);

             bodyMeshes_[bi].forcesFO().calcForcesMoment();

//            Foam::tmp<Foam::volScalarField> rho;
            
//            {
//                if (rhoName_ == "rhoInf")
//                {
//                    rho=tmp<volScalarField>
//                    (
//                        new volScalarField
//                        (
//                            IOobject
//                            (
//                                "rho",
//                                fvmesh.time().timeName(),
//                                fvmesh
//                            ),
//                            fvmesh,
//                            dimensionedScalar("rho", dimDensity, rhoInf_)
//                        )
//                    );
//                }
//                else
//                {
//                    rho=tmp<volScalarField>(
//                                new volScalarField(
//                                    fvmesh.lookupObject<volScalarField>(
//                                        rhoName_) ) );
//                }
//            }

//            filterPatchType(
//                        fvmesh,
//                        bodyMeshes_[bi].patches_,
//                        wallFvPatch::typeName,
//                        true )
//            patchMomentumForce pmf
//            (
//                fvmesh,
//                ,
//                rho/*,
//                referenceSystemSpeed_*/
//            );

            auto transformedPoint = [&](const vector& v) {
                return model_.transformPoints(
                            bodyID,
                            pointField(1, v)
                            )()[0];
            };

#warning assumes z upwards
            auto yawedVector = [&](const vector& v) {
                vector y=-v ^ vector(0,0,1);
                y/=mag(y);
                return y^vector(0,0,1);
            };

            point orgCtr=model_.bodies()[bodyID].c();
            point curCtr = transformedPoint( orgCtr );
            // calculate direct forces
            auto rotatedMotionDirection =
                    yawedVector(
                        transformedPoint(body.globalMotionDirection_+orgCtr)-curCtr
                        );

            scalar R = body.forcesFO().forceEff() & rotatedMotionDirection;
            vector dF=vector::zero, dM=vector::zero;
            forAll(body.directThrustForces_, j)
            {
                const directThrustForce& df = body.directThrustForces_[j];

//                point cur_poa = transformedPoint( df.PoA_ );

//                scalar F = mag(df.direction_);
//                if (df.resistance_fraction_)
//                    F = -R * (*df.resistance_fraction_);

//                vector cur_dir=transformedPoint( df.direction_/F + df.PoA_ ) - cur_poa;

//                vector ddf = (F / (cur_dir & rotatedMotionDirection )) * cur_dir; // scale force
//                vector ddm = ( cur_poa - cur_ctr ) ^ ddf;

                auto pf = df.PoAAndForce(transformedPoint);
                vector ddf = pf.second;
                vector ddm = ( pf.first - curCtr ) ^ ddf;

                //                Info<<"Direct force "<<j<<":"<<endl;
                //                Info<<" Motion direction : "<<body.globalMotionDirection_<<" => "<<rotatedMotionDirection<<endl;
                //                Info<<" PoA : "<<df.PoA_<<" => "<<cur_poa<<endl;
                //                Info<<" Dir : "<<df.direction_<<" => "<<cur_dir<<endl;
                //                Info<<" Body Ctr : "<<ctr<<" => "<<cur_ctr<<endl;

                Info << "Direct force " << j
                     << ": force = " << ddf << "(" << mag(ddf) << ")"
                     << ", moment = " << ddm << "(" << mag(ddm) << ")"
                     << endl;

                dF+=ddf;
                dM+=ddm;
            }

            vector pmF=vector::zero, pmM=vector::zero;
            forAll(body.patchMomentumForces_, k)
            {
                auto pFM=body.patchMomentumForces_[k].forceAndMoment();
                pmF+=pFM.first;
                pmM+=pFM.second;
            }
            
            fx[bodyID] = spatialVector
            ( 
                ramp*(body.forcesFO().momentEff() + /*pmf.moment()*/ pmM + dM),
                ramp*(body.forcesFO().forceEff() + /*pmf.force()*/ pmF + dF)
            );
            
            // patch momentum force influence
            {
                vector mfrac=1e2*cmptDivide(/*pmf.moment()*/pmM, body.forcesFO().momentEff());
                vector ffrac=1e2*cmptDivide(/*pmf.force()*/pmF, body.forcesFO().forceEff());
                Info
                    << boost::str(boost::format(
                                  "[%01d]: PMF %10s%10s%10s%10s%10s%10s\n" )
                                 % bodyID
                                 % "Fx/Fxhull" % "Fy/Fyhull" % "Fz/Fzhull"
                                 % "Mx/Mxhull" % "My/Myhull" % "Mz/Mzhull" ).c_str()
                    << boost::str(boost::format(
                                  "[%01d]:      %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%%\n" )
                                 % bodyID
                                 % ffrac.x() % ffrac.y() % ffrac.z()
                                 % mfrac.x() % mfrac.y() % mfrac.z() ).c_str()
                    ;
            }
            
            // direct force influence
            {
                vector mfrac=1e2*cmptDivide(dM, body.forcesFO().momentEff());
                vector ffrac=1e2*cmptDivide(dF, body.forcesFO().forceEff());
                Info
                    << boost::str(boost::format(
                                   "[%01d]:  DF %10s%10s%10s%10s%10s%10s\n" )
                                  % bodyID
                                  % "Fx/Fxhull" % "Fy/Fyhull" % "Fz/Fzhull"
                                  % "Mx/Mxhull" % "My/Myhull" % "Mz/Mzhull" ).c_str()
                    << boost::str(boost::format(
                                   "[%01d]:      %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%% %8.1f%%\n" )
                                  % bodyID
                                  % ffrac.x() % ffrac.y() % ffrac.z()
                                  % mfrac.x() % mfrac.y() % mfrac.z() ).c_str()
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
                mesh().time().timeName(
                        mesh().time().startTime().value() );

            if (Pstream::parRun())
            {
                // Put in undecomposed case (Note: gives problems for
                // distributed data running)
                outdir =
                        mesh().time().path()/
                        ".."/
                        "postProcessing"/
                        "rigidBodyMotion"/
                        startTimeName;
            }
            else
            {
                outdir =
                        mesh().time().path()/
                        "postProcessing"/
                        "rigidBodyMotion"/
                        startTimeName;
            }

            // Create directory if does not exist.
            mkDir(outdir);

            // Open new file at start up
            filePtr_.reset(new OFstream(
                               outdir/
                               "rigidBodyMotion.dat")
                           );

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
