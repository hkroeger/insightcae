#ifndef SRFOPTION_H
#define SRFOPTION_H

#include "fvOption.H"
#include "SRFModel.H"

namespace Foam
{
namespace fv
{

class SRFoption
: public option
{

  autoPtr<SRF::SRFModel> srf_;

public:
  TypeName("SRFoption");

  SRFoption(const word & 	name,
            const word & 	modelType,
            const dictionary & 	dict,
            const fvMesh & 	mesh );


  virtual void addSup
  (
      fvMatrix<vector>& eqn,
      const label fieldi
  );


  virtual void addSup
  (
      const volScalarField& rho,
      fvMatrix<vector>& eqn,
      const label fieldi
  );

};

}
}

#endif // SRFOPTION_H
