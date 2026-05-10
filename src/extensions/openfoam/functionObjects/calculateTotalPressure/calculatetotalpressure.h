#ifndef CALCULATETOTALPRESSURE_H
#define CALCULATETOTALPRESSURE_H

#include <memory>
#include <functional>

#include "uniof_functionobject.h"
#include "volFields.H"


namespace Foam {

class calculateTotalPressure
    : public UniFunctionObject
{
private:
    const fvMesh& mesh_;

    std::unique_ptr<volScalarField> rhoInf_;

    const volScalarField *rho_;
    const volScalarField *p_;
    const volVectorField *U_;

    dimensionedScalar pAmbient_;
    volScalarField pTotal_;

public:
    TypeName("calculateTotalPressure");

    calculateTotalPressure(
        const word& name,
        const objectRegistry& obr,
        const dictionary& dict );

    bool read(const dictionary&) override;
    bool perform() override;
    bool write() override;
};

} // namespace Foam

#endif
