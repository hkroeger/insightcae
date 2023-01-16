#ifndef INSIGHT_LSDYNAINPUTCARDS_MATERIALS_H
#define INSIGHT_LSDYNAINPUTCARDS_MATERIALS_H

#include "lsdyna/lsdynainputcard.h"
#include "base/units.h"


namespace insight {
namespace LSDynaInputCards {





class Material
        : public InputCardWithId
{
public:
    Material(int id);
};


class MatRigid
        : public Material
{
public:
    enum DOF {
        Free = 0,
        X = 1,
        Y = 1,
        Z = 3,
        XY = 4,
        YZ = 5,
        XZ = 6,
        XYZ = 7
    };
private:
    boost::units::quantity<si::mass_density, double> contactDensity_;
    boost::units::quantity<si::pressure, double> contactStiffness_;
    double n_;

    DOF translationBC_, rotationBC_;

public:
    MatRigid(
            int id,
            const boost::units::quantity<si::mass_density, double>& contactDensity,
            const si::Pressure& contactStiffness,
            double n = 0,
            DOF translationBC = DOF::Free,
            DOF rotationBC = DOF::Free );

    void write(std::ostream& os) const override;
};




class MatElastic
        : public Material
{
    boost::units::quantity<si::mass_density, double> density_;
    boost::units::quantity<si::pressure, double> E_;
    double nu_;

public:
    MatElastic(
            int id,
            const boost::units::quantity<si::mass_density, double>& density,
            const si::Pressure& E,
            double nu );

    void write(std::ostream& os) const override;
};




class MatPlasticKinematic
        : public Material
{
    boost::units::quantity<si::mass_density, double> density_;
    boost::units::quantity<si::pressure, double> Eelas_, yieldStress_, plasticSlope_;
    double nu_, beta_;

public:
    MatPlasticKinematic(
            int id,
            const boost::units::quantity<si::mass_density, double>& density,
            const si::Pressure& Eelas,
            double nu,
            const si::Pressure& yieldStress,
            const si::Pressure& plasticSlope,
            double beta = 1 );

    void write(std::ostream& os) const override;
};


} // namespace LSDynaInputCards
} // namespace insight

#endif // INSIGHT_LSDYNAINPUTCARDS_MATERIALS_H
