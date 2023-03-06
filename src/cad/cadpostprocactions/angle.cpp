#include "angle.h"
#include "cadfeature.h"
#include "base/units.h"

namespace insight {
namespace cad {


defineType(Angle);

size_t Angle::calcHash() const
{
  ParameterListHash h;
  h+=p1_->value();
  h+=p2_->value();
  h+=pCtr_->value();
  return h.getHash();
}


Angle::Angle(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2, insight::cad::VectorPtr pCtr)
: p1_(p1), p2_(p2), pCtr_(pCtr)
{}


double Angle::calculate(
        arma::mat p1,
        arma::mat p2,
        arma::mat pCtr
        )
{
    arma::mat d1=p1-pCtr;
    arma::mat d2=p2-pCtr;

    arma::mat n=arma::cross(d1, d2);
    if (arma::norm(n,2)<SMALL)
    {
        return 0.0;
    }
    else
    {
        arma::mat ex=normalized(d1);
        arma::mat ey=normalized(arma::cross(n,ex));
        return atan2(arma::dot(d2, ey), arma::dot(d2, ex));
    }
}


void Angle::build()
{
    angle_ = calculate(
                p1_->value(),
                p2_->value(),
                pCtr_->value() );

    cout<<"######### Angle Report ###########################################"<<endl;
    cout<<"angle="<<angle_/SI::deg<<"deg"<<endl;
}

void Angle::write(ostream&) const
{}





defineType(AngleConstraint);

size_t AngleConstraint::calcHash() const
{
    ParameterListHash h;
    h+=p1_->value();
    h+=p2_->value();
    h+=pCtr_->value();
    h+=targetValue_->value();
    return h.getHash();
}


AngleConstraint::AngleConstraint(VectorPtr p1, VectorPtr p2, VectorPtr pCtr, ScalarPtr targetValue)
    : Angle(p1, p2, pCtr),
      targetValue_(targetValue)
{}


ScalarPtr AngleConstraint::targetValue()
{
    return targetValue_;
}


int AngleConstraint::nConstraints() const
{
    return 1;
}

double AngleConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
                iConstraint==0,
                "invalid constraint id" );
    checkForBuildDuringAccess();
    return angle_ - targetValue_->value();
}




} // namespace cad
} // namespace insight
