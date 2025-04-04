
#include "cadfeatures.h"
#include "cadparameters/constantvector.h"

using namespace insight;
using namespace insight::cad;

int main(int argc, char* argv[])
{
    try
    {
        const std::string passiveWallsName="passiveWalls";
        const std::string airExchangeSurfaceName="airExchangeSurface";

        auto geo = cad::Cylinder::create(
            cad::matconst(vec3(0,0,0)),
            cad::matconst(vec3(0,0,1)),
            cad::scalarconst(1),
            false, false );

        auto exf = insight::cad::makeFaceFeatureSet(
            geo, "maximal(area)");
        auto wallf = insight::cad::makeFaceFeatureSet(
            geo, "!in(%0)",
            {exf} );

        auto ex = cad::Import::create(exf);
        auto w = cad::Import::create(wallf);
        std::cout<<"PW_0 ("<<w->hash()<<")"<<std::endl;
        // BRepTools::Dump(pwn->shape(), std::cout);
        std::cout<<"AES_0 ("<<ex->hash()<<")"<<std::endl;

        auto vp = cad::Compound::create(
            cad::CompoundFeatureMap
            {
                { passiveWallsName,
                 cad::Import::create(wallf) },
                { airExchangeSurfaceName,
                 cad::Import::create(exf) }
            } );

        // auto vp = cad::Transform::create(
        //         baseGeometry_,
        //         cad::matconst(vec3(1,1,1)),
        //         cad::matconst(vec3(0,0,45.))
        //     );

        auto pwn = vp->subshape(passiveWallsName);
        auto aes = vp->subshape(airExchangeSurfaceName);

        std::cout<<"PW ("<<pwn->hash()<<")"<<std::endl;
        // BRepTools::Dump(pwn->shape(), std::cout);
        std::cout<<"AES ("<<aes->hash()<<")"<<std::endl;
        // BRepTools::Dump(aes->shape(), std::cout);

        insight::assertion(
         !pwn->shape().IsEqual(aes->shape()),
            "both shapes must not be identical!");

    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
