#include "base/exception.h"
#include "base/units.h"
#include "base/casedirectory.h"

#include "lsdyna/lsdynainputdeck.h"

#include "lsdyna/materials.h"
#include "lsdyna/solution.h"
#include "lsdyna/control.h"

using namespace insight;

namespace boost { namespace units { namespace si {

template<class Dimension, class Type, class Unit>
Type toValue2(const quantity<Dimension,Type>& q, const Unit& u)
{
  return static_cast<quantity<si::dimensionless, Type> >( q / u ).value();
}

}}}


int main(int /*argc*/, char*/*argv*/[])
{
    try
    {
        CaseDirectory dir(false);

        using namespace LSDynaInputCards;
        LSDynaInputDeck key(
                    LSDynaInputCards::Control_Units(
                        si::millimeter, si::seconds, si::metric_ton ));

        key.append(new Control_Timestep(0.9, si::Time(1*si::micro*si::second)));
        auto nodes = key.append(new Node(
           Node::NodeList{
               {1, si::LengthVector(vec3(0,   0,    0), si::millimeter)},
               {2, si::LengthVector(vec3(0.1, 0,    0), si::centi*si::meter)},
               {3, si::LengthVector(vec3(0,   1e-3, 0), si::meter)},
               {4, si::LengthVector(vec3(1,   1,    0), si::millimeter)}
           }));

        auto mat = key.append(new MatElastic(
                    1,
                    7900.*si::kilogram_per_cubic_meter,
                    si::Pressure(250000.*si::mega*si::pascals),
                    0.25 ));

        auto sec = key.append(new SectionShell(1, si::Length(1*si::millimeter)));
        auto part = key.append(new Part("part", 1, sec, mat));

        key.append(new ElementShell(
                       {
                           {1, {part, {1,2,3,-1}}},
                           {2, {part, {2,4,3,-1}}}
                       }));

        key.append(new Curve(
                       1,
                       {
                           {0,0},
                           {1,1}
                       }));

        auto infile = dir/"input.key";
        std::ofstream f(infile.string());
        f<<key;
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
