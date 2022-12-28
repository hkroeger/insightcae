#include "coordinatesystems.h"

#include "base/exception.h"
#include "base/linearalgebra.h"

insight::BeamLocalCS::BeamLocalCS(const arma::mat& origin, const arma::mat& alongBeamDir)
{
    arma::mat
            ex=normalized(alongBeamDir),
            gez=vec3Z(1);

    arma::mat ey=arma::cross(gez, ex);
    if ( arma::norm(ey,2)<SMALL )
    {
        double d=arma::dot(gez,ex);
        if (d>0.)
            ey=vec3Y(1);
        else if (d<0.)
            ey=vec3Y(-1);
        else
            throw insight::Exception("internal error");
    }
    else
    {
        ey=normalized(ey);
    }

    arma::mat ez=arma::cross(ex, ey);

    std::cout<<ex.t()<<" "<<ey.t()<<" "<<ez.t()<<std::endl;

    R_.col(0)=ex;
    R_.col(1)=ey;
    R_.col(2)=ez;
    translate_ = arma::inv(R_)*origin;
}

insight::SpatialTransformation insight::BeamLocalCS::MACRCARAPOUTRE() const
{
    SpatialTransformation st(
                vec3Y(1), vec3Z(1), vec3X(1) );
    st.appendTransformation(*this);
    return st;
}
