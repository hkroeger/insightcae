#include "materials.h"
#include "lsdyna/lsdynainputdeck.h"


namespace insight {
namespace LSDynaInputCards {




Material::Material(int id)
    : InputCardWithId(id)
{}




MatRigid::MatRigid(
        int id,
        const boost::units::quantity<si::mass_density, double>& contactDensity,
        const si::Pressure& contactStiffness,
        double n,
        DOF translationBC,
        DOF rotationBC )
    : Material(id),
      contactDensity_(contactDensity),
      contactStiffness_(contactStiffness),
      n_(n),
      translationBC_(translationBC),
      rotationBC_(rotationBC)
{}

void MatRigid::write(std::ostream& os) const
{
    os << "*MAT_RIGID\n";
    os << id() <<", "
       << toValue(contactDensity_, inputDeck().densityUnit()) <<", "
       << toValue(contactStiffness_, inputDeck().stressUnit()) << ", "
       << n_ << "\n";
    os << ( ( translationBC_!=DOF::Free || rotationBC_!=DOF::Free )? 1 : 0 ) << ", "
       << int(translationBC_) << ", "
       << int(rotationBC_) << "\n";

    os << "\n";

}




MatElastic::MatElastic(
        int id,
        const boost::units::quantity<si::mass_density, double>& density,
        const boost::units::quantity<si::pressure, double>& E,
        double nu )
    : Material(id),
      density_(density), E_(E), nu_(nu)
{}

void MatElastic::write(std::ostream &os) const
{
    os << "*MAT_ELASTIC\n";
    os << id() <<", "
       << toValue(density_, inputDeck().densityUnit()) <<", "
       << toValue(E_, inputDeck().stressUnit()) <<", "
       << nu_ << "\n";
}





MatPlasticKinematic::MatPlasticKinematic(
        int id,
        const boost::units::quantity<si::mass_density, double>& density,
        const si::Pressure& Eelas,
        double nu,
        const si::Pressure& yieldStress,
        const si::Pressure& plasticSlope,
        double beta )
    : Material(id),
      density_(density),
      Eelas_(Eelas),
      yieldStress_(yieldStress),
      plasticSlope_(plasticSlope),
      nu_(nu), beta_(beta)
{}

void MatPlasticKinematic::write(std::ostream& os) const
{
    os << "*MAT_PLASTIC_KINEMATIC\n";
    os << id() <<", "
       << toValue(density_, inputDeck().densityUnit()) <<", "
       << toValue(Eelas_, inputDeck().stressUnit()) <<", "
       << nu_ << ", "
       << toValue(yieldStress_, inputDeck().stressUnit()) <<", "
       << toValue(plasticSlope_, inputDeck().stressUnit()) <<", "
       << beta_ << "\n";
    os << "\n";
}



} // namespace LSDynaInputCards
} // namespace insight
