#include <iostream>

#include "base/casedirectory.h"

#include "cadfeatures/quad.h"
#include "cadfeatures/box.h"
#include "meshing.h"

using namespace insight;
using namespace insight::cad;

template<class T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s)
{
    for (const auto& i: s)
        os<<" "<<i;
    return os;
}

int main(int, char*[])
{
    try
    {
        CaseDirectory dir(false);

        {
            auto surface = Quad::create(
                        matconst(vec3(0,0,0)),
                        matconst(vec3(1,0,0)),
                        matconst(vec3(0,1,0)) );

            boost::filesystem::path meshFile = dir/"mesh.key";
            cad::SurfaceGmshCase c(
                        surface, meshFile,
                        0.1,
                        0.1,
                        "surface",
                        false, false
                        );
            c.doMeshing();

            auto spis=c.getLSDynaShellPartIDs("surface");
            std::cout<<"part IDs ="<<spis<<std::endl;

            insight::assertion(
                        boost::filesystem::exists(meshFile),
                        "meshing was not successful"
                        );
        }

        {
            auto box = Box::create(
                        matconst(vec3(0,0,0)),
                        matconst(vec3(1,0,0)),
                        matconst(vec3(0,1,0)),
                        matconst(vec3(0,0,1)) );

            auto inlet=makeFaceFeatureSet(box, "minimal(CoG.x)");
            auto outlet=makeFaceFeatureSet(box, "maximal(CoG.x)");
            auto sides=makeFaceFeatureSet(box, "!(in(%0)||in(%1))",
                                        {inlet, outlet});
            auto vol = box->allSolids();

            boost::filesystem::path meshFile = dir/"mesh_box.key";
            cad::GmshCase c(
                        box, meshFile
                        );
            c.nameFaces("inlet", *inlet);
            c.nameFaces("outlet", *outlet);
            c.nameFaces("sides", *sides);
            c.nameSolids("box", *vol);
            c.doMeshing();

            std::cout<<"inlet part IDs ="<<c.getLSDynaShellPartIDs("inlet")<<std::endl;
            std::cout<<"outlet part IDs ="<<c.getLSDynaShellPartIDs("outlet")<<std::endl;
            std::cout<<"sides part IDs ="<<c.getLSDynaShellPartIDs("sides")<<std::endl;
            std::cout<<"solid IDs ="<<c.getLSDynaSolidPartIDs("box")<<std::endl;

            insight::assertion(
                        boost::filesystem::exists(meshFile),
                        "meshing was not successful"
                        );
        }
    }

    catch (const std::exception& e)
    {
      std::cerr<<e.what()<<std::endl;
      return -1;
    }

    return 0;
}
