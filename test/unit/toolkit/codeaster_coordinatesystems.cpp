#include "base/linearalgebra.h"
#include "base/exception.h"
#include "code_aster/coordinatesystems.h"

using namespace insight;

int main(int, char*[])
{
    arma::mat
            somePtInPlane = vec3(0,2,3),
            someOfs = vec3X(5.);

    {
        std::string tcn("identity transform");
        BeamLocalCS b( vec3Zero(), vec3X(1) );
        std::cout<<tcn<<":\n "<<b.trsfPt(somePtInPlane).t()<<" "<<somePtInPlane.t()<<std::endl;
        insight::assertion(
                arma::norm(b.trsfPt(somePtInPlane)-somePtInPlane, 2)<SMALL,
                tcn+" failed");
    }

    {
        std::string tcn("longitudinal offset");
        BeamLocalCS b( someOfs, vec3X(1) );
        arma::mat gpt = b.trsfPt(somePtInPlane);
        std::cout<<tcn<<":\n "<<gpt.t()<<" "<<somePtInPlane.t()<<std::endl;
        insight::assertion(
                arma::norm(b.trsfPt(vec3Zero())-someOfs, 2)<SMALL,
                "offset identity transform failed");
        insight::assertion(
                arma::norm(vec3(0, gpt(1)-somePtInPlane(1), gpt(2)-somePtInPlane(2)), 2)<SMALL,
                tcn+" failed");
    }

    {
        arma::mat ofs=vec3(2,3,4), dir=vec3(1,1,0);
        arma::mat expect =
                ofs
                + vec3(-cos(M_PI/4),sin(M_PI/4),0)*somePtInPlane(1)
                + vec3(0,0,1)*somePtInPlane(2);
        std::string tcn("complete transform 1");
        BeamLocalCS b( ofs, dir );
        std::cout<<tcn<<":\n "<<b.trsfPt(somePtInPlane).t()<<" "<<somePtInPlane.t()<<" "<<expect.t()<<std::endl;
        insight::assertion(
                arma::norm(b.trsfPt(somePtInPlane)-expect, 2)<SMALL,
                tcn+" failed");
    }

    return 0;
}
