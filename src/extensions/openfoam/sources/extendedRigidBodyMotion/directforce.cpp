#include "directforce.h"

#include "dictionary.H"

#include "globalcoordinatesystems.h"

namespace Foam {
namespace extRBM {


directForceFactory::directForceFactory(const Time& time)
    : time_(time)
{}

autoPtr<directForce> directForceFactory::operator()(Istream& is) const
{
    dictionary d(is);
    autoPtr<directForce> df
    (
        new directForce
        (
            point(d.lookup("PoA")),
            vector(d.lookupOrDefault<vector>("localDirection", vector::zero)),
            vector(d.lookupOrDefault<vector>("verticalDirection", vector::zero)),
            forceSourceCombination(time_, d.lookup("forceSource")),
            d.lookupOrDefault<word>("coordinateSystemName", "global")
        )
    );

    return df;
}

//autoPtr<directForce>
//directForce::New(Istream& is)
//{

//}


defineTypeNameAndDebug(directForce, 0);


directForce::directForce(
        const point& p,
        const vector& d,
        const vector& v,
        const forceSourceCombination& fs,
        const word& coordinateSystemName
        )

: PoA_(p),
  localDirection_(d),
  verticalConstraint_(v),
  fs_(fs),
  coordinateSystemName_(coordinateSystemName)
{}


//std::pair<point,vector> directThrustForce::PoAAndForce
//( std::function<point(point)> transformedPoint ) const

std::pair<vector,vector> directForce::forceAndMoment() const
{
    auto cs = coordinateSystemSources::registry()
            .get(coordinateSystemName_)
            ->getCoordinateSystem();

//    point curPoA = transformedPoint( PoA_ );
    auto curPoA = cs->globalPosition(PoA_);

    vector Forg=fs_.force(), F=Forg;

    Info<<"before trs F= ^"<<Forg;

    scalar m=mag(localDirection_);
    if (m>SMALL)
    {
        vector curDir=cs->globalVector(localDirection_/m); //transformedPoint( localDirection_/m + PoA_ ) - curPoA;
        curDir/=mag(curDir);

        F = curDir * (Forg & curDir);

        scalar mv=mag(verticalConstraint_);
        if (mv>SMALL)
        {
            vector eq = curDir^verticalConstraint_;
            vector el = verticalConstraint_^eq;
            el/=mag(el);

            scalar fplanereq=Forg&el;
            scalar fplanecur=F&el;

            F*=fplanereq/fplanecur;
        }
    }
    Info<<", after trs F= "<<F<<", PoA= "<<curPoA<<endl;
    return {F, curPoA^F};
}




autoPtr<directForce>
directForce::clone() const
{
  return autoPtr<directForce>(
              new directForce(
                  PoA_,
                  localDirection_,
                  verticalConstraint_,
                  fs_,
                  coordinateSystemName_) );
}



} // namespace extRBM
} // namespace Foam
