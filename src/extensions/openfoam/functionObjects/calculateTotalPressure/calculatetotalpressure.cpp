#include "calculatetotalpressure.h"
#include "addToRunTimeSelectionTable.H"


namespace Foam {



defineTypeNameAndDebug(calculateTotalPressure, 0);

addToRunTimeSelectionTable(
    functionObject,
    calculateTotalPressure,
    dictionary
    );



calculateTotalPressure::calculateTotalPressure(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict )
    : UniFunctionObject(name, dict),
    mesh_(UNIOF_OBR_TO_MESH(obr)),
    pAmbient_("pAmbient", dimPressure, 0.),
    pTotal_(
        IOobject(
            "pTotal",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
            ),
        mesh_,
        pAmbient_
        )
{}

bool calculateTotalPressure::read(const dictionary &dict)
{
    pAmbient_=dict.lookupOrDefault<dimensionedScalar>(
        "pAmbient", dimensionedScalar("pAmbientDefl", dimPressure, 0.));

    pName_ = dict.lookupOrDefault<word>("pName", "p");
    UName_ = dict.lookupOrDefault<word>("UName", "U");
    rhoName_ = dict.lookupOrDefault<word>("rhoName", "rho");
    rhoInfValue_ = dict.lookupOrDefault<scalar>("rhoInf", 0.);

    return true;
}


const volScalarField& calculateTotalPressure::p() const
{
    if (!mesh_.foundObject<volScalarField>( pName_ ))
        FatalErrorIn("calculateTotalPressure::read")
            <<"no field "<<pName_<<abort(FatalError);
    return mesh_.lookupObject<volScalarField>( pName_ );
}

const volVectorField& calculateTotalPressure::U() const
{
    if (!mesh_.foundObject<volVectorField>( UName_ ))
        FatalErrorIn("calculateTotalPressure::read")
            <<"no field U"<<abort(FatalError);
    return mesh_.lookupObject<volVectorField>( UName_ );
}

tmp<volScalarField> calculateTotalPressure::rho() const
{
    if (rhoName_=="rhoInf")
    {

        return tmp<volScalarField>(
            new volScalarField
            (
                IOobject
                (
                    "rho",
                    mesh_.time().timeName(),
                    mesh_
                    ),
                mesh_,
                dimensionedScalar("rho", dimDensity, rhoInfValue_)
                ));
    }
    else
    {
        if (!mesh_.foundObject<volScalarField>(rhoName_))
            FatalErrorIn("calculateTotalPressure::read")
                <<"no field rho with name "<<rhoName_<<abort(FatalError);
        return mesh_.lookupObject<volScalarField>(rhoName_);
    }
}



bool calculateTotalPressure::perform()
{
    tmp<volScalarField> pFactor;
    if (p().dimensions() == dimPressure)
    {
        pFactor=tmp<volScalarField>(
          new volScalarField(
            IOobject(
                "pfac",
                mesh_.time().timeName(),
                mesh_
                ),
            mesh_,
            dimensionedScalar("", dimless, 1.)
        ));
    }
    else
    {
        pFactor=rho();
    }

    pTotal_ = pAmbient_
              + p() * pFactor
              + 0.5*rho()*sqr( mag(U()) );

    return true;
}

bool calculateTotalPressure::write()
{
    return true;
}

}
