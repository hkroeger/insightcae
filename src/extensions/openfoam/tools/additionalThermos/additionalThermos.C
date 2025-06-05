

#include "rhoThermo.H"
#include "makeThermo.H"

#include "specie.H"
#include "rhoConst.H"

#include "hConstThermo.H"
#include "eConstThermo.H"
#include "sensibleEnthalpy.H"
#include "thermo.H"

#include "WLFTransport.H"

#include "icoPolynomial.H"

#include "heRhoThermo.H"
#include "pureMixture.H"


namespace Foam
{


makeThermos
(
    rhoThermo,
    heRhoThermo,
    pureMixture,
    WLFTransport,
    sensibleEnthalpy,
    hConstThermo,
    icoPolynomial,
    specie
);


}
