#include "fvCFD.H"
#include "uniof.h"
#include <functional>

using namespace Foam;


int main(int argc, char* argv[])
{
    argList::validArgs.append("turbulence intensity");
    argList::validArgs.append("turbulent length scale");

    argList::validOptions.insert("k", "");
    argList::validOptions.insert("epsilon", "");
    argList::validOptions.insert("omega", "");
    argList::validOptions.insert("nut", "");

# include "setRootCase.H"
# include "createTime.H"
# include "createMesh.H"

    auto I = readScalar(IStringStream(UNIOF_ADDARG(args, 0))());
    auto L = readScalar(IStringStream(UNIOF_ADDARG(args, 1))());

    volVectorField U(
        IOobject(
            "U",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
            ),
        mesh
    );



    auto apply = [&](
        volScalarField& fld,
        std::function<scalar(const vector&)> func)
    {
        forAll(fld, i)
        {
            fld[i]=func(U[i]);
        }
        forAll(fld.boundaryField(), j)
        {
            forAll(fld.boundaryField()[j], k)
            {
                UNIOF_BOUNDARY_NONCONST(fld)[j][k] ==
                    func(U.boundaryField()[j][k]);
            }
        }
    };

    auto doFor = [&](const std::string& fldName,
                     std::function<scalar(const vector&)> func)
    {
        if (UNIOF_OPTIONFOUND(args, fldName))
        {
            volScalarField fld(
                IOobject(
                    fldName,
                    runTime.timeName(),
                    mesh,
                    IOobject::MUST_READ,
                    IOobject::AUTO_WRITE
                    ),
                mesh
                );

            apply(fld, func);

            Info << "Writing field " << fldName << endl;
            fld.write();
        }
    };

    scalar Cmu=0.09;
    scalar Cmu75=Foam::pow(Cmu, 0.75);

    auto k = [&](const vector& U)
    {
        return std::max(SMALL, 1.5*sqr(I)*magSqr(U));
    };

    doFor("k", k);


    doFor("epsilon", [&](const vector& U)
          {
              return (Cmu75/L)*Foam::pow(k(U), 1.5);
          });

    doFor("omega", [&](const vector& U)
          {
              return Foam::sqrt(k(U))/(Cmu75*L);
          });


    doFor("nut", [&](const vector& U)
          {
              scalar kp=k(U);
              scalar eps=std::max(SMALL, (Cmu75/L)*Foam::pow(kp, 1.5));
              return Cmu*sqr(kp)/eps;
          });

    Info << "End." << endl;
}
