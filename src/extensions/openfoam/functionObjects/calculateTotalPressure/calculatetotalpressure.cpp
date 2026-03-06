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
    p_ = &mesh_.lookupObject<volScalarField>(
        dict.lookupOrDefault<word>("pName", "p") );
    U_ = &mesh_.lookupObject<volVectorField>(
        dict.lookupOrDefault<word>("UName", "U") );

    auto rhoName=dict.lookupOrDefault<word>("rhoName", "rho");
    if (rhoName=="rhoInf")
    {
        scalar rhoInf=readScalar(dict.lookup("rhoInf"));
        rhoInf_.reset(
            new volScalarField
            (
                IOobject
                (
                    "rho",
                    mesh_.time().timeName(),
                    mesh_
                    ),
                mesh_,
                dimensionedScalar("rho", dimDensity, rhoInf)
                ));
        rho_=rhoInf_.get();
    }
    else
    {
        rho_ = &mesh_.lookupObject<volScalarField>(rhoName);
    }
    return true;
}



bool calculateTotalPressure::perform()
{
    auto &p=*p_; auto& U=*U_; auto& rho=*rho_;

    tmp<volScalarField> pFactor;
    if (p.dimensions() == dimPressure)
    {
        pFactor=volScalarField(
            IOobject(
                "pfac",
                mesh_.time().timeName(),
                mesh_
                ),
            mesh_,
            dimensionedScalar("", dimless, 1.)
            );
    }
    else
    {
        pFactor=rho;
    }

    pTotal_ = pAmbient_
              + p * pFactor
              + 0.5*rho*sqr( mag(U) );

    return true;
}

bool calculateTotalPressure::write()
{
    return true;
}

}
