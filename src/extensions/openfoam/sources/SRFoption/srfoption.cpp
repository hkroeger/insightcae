#include "srfoption.h"
#include "addToRunTimeSelectionTable.H"

#include "fvMatrix.H"

namespace Foam
{
namespace fv
{

defineTypeNameAndDebug(SRFoption, 0);
addToRunTimeSelectionTable
(
    option,
    SRFoption,
    dictionary
);

SRFoption::SRFoption(const word & 	name,
                     const word & 	modelType,
                     const dictionary & dict,
                     const fvMesh & 	mesh
)
: option(name, modelType, dict, mesh)
{
  fieldNames_= wordList(1, "U");
  applied_.setSize(fieldNames_.size(), false);
}


//autoPtr<option> SRFoption::clone() const
//{
//    return return autoPtr<option>(new SRFoption(name(), mode;
//}

void SRFoption::addSup
(
    fvMatrix<vector>& eqn,
    const label
)
{
  if (!srf_.valid())
  {
    Info<<"creating SRF model"<<endl;
    srf_ = SRF::SRFModel::New(eqn.psi());
  }

  Info<<"apply source"<<endl;
  eqn -= srf_->Su();
}

void SRFoption::addSup
(
    const volScalarField& rho,
    fvMatrix<vector>& eqn,
    const label
)
{
  if (!srf_.valid())
  {
    Info<<"creating SRF model"<<endl;
    srf_ = SRF::SRFModel::New(eqn.psi());
  }

  Info<<"apply source"<<endl;
  eqn -= rho*srf_->Su();
}

}
}
