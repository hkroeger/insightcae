#include <iostream>

#include "base/exception.h"
#include "openfoam/ofpatchcadfeature.h"
#include "cadfeature.h"

#include "BRep_Tool.hxx"
#include "TopoDS.hxx"
#include "Poly_Triangulation.hxx"

using namespace insight;


int main(int argc, char*argv[])
{
    try
    {
        insight::assertion(
            argc==2,
            "specify source dir with Openfoam case as argument!");

        auto caseDir = boost::filesystem::path(argv[1])/"tutorial_mixerVesselAMI2D";

        auto feat = cad::readOpenFOAMPatchAsFeature(caseDir, "rotor");

        TopoDS_Shape s = feat->shape();
        insight::assertion(s.ShapeType()==TopAbs_FACE, "expected a single face");

        TopLoc_Location loc;
        auto tri = BRep_Tool::Triangulation(TopoDS::Face(s), loc);
        insight::assertion(!tri.IsNull(), "no triangulation present");

        std::cout<<"nodes="<<tri->NbNodes()<<", triangles="<<tri->NbTriangles()<<std::endl;

        // 192 quads with 384 points; STL splits points at sharp edges
        insight::assertion(tri->NbNodes()>=384, "unexpected number of nodes");
        insight::assertion(tri->NbTriangles()==2*192, "unexpected number of triangles");
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
