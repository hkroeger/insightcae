
#include "base/exception.h"
#include "base/boost_include.h"

#include "base/linearalgebra.h"
#include "base/vtkrendering.h"

#include "vtkGenericDataObjectReader.h"
#include "vtkMeshQuality.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkCellCenters.h"
#include "vtkCellSizeFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include <iostream>



class OptimalViews
{
public:
    OptimalViews(vtkPointSet *ps)
    {
        auto es = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        es->SetInputData(ps);
        es->Update();

        // es->GetOutput()->Print(std::cout);

        auto ms = vtkSmartPointer<vtkCellSizeFilter>::New();
        ms->ComputeAreaOn();
        ms->SetInputData(es->GetOutput());
        ms->Update();

        // ms->GetOutput()->Print(std::cout);


        auto normals=vtkSmartPointer<vtkPolyDataNormals>::New();
        normals->SetInputData(ms->GetOutput());
        normals->ComputeCellNormalsOn();
        normals->SplittingOn();
        normals->ConsistencyOn();
        normals->Update();

        // normals->GetOutput()->Print(std::cout);

        auto ctrs = vtkSmartPointer<vtkCellCenters>::New();
        ctrs->SetInputData(normals->GetOutput());
        ctrs->Update();


        auto data = ctrs->GetOutput();

        data->Print(std::cout);

        double Atot=0.;
        {
            auto pd=data->GetPointData();
            auto ps=data->GetPoints();
            auto ns=pd->GetArray("Normals");
            auto As=pd->GetArray("Area");
            for (int i=0; i<data->GetNumberOfPoints(); ++i)
            {
                arma::mat n = -insight::normalized(
                    insight::vec3FromComponents(ns->GetTuple3(i)));
                double A = As->GetTuple1(i);
                Atot += A;

                double theta = acos(n(2));
                double st = sin(theta);
                double sp = n(1)/st;
                double cp = n(0)/st;
                double phi = atan2(sp, cp);


            }
        }

        // for (int i=0; i<ps->GetNumberOfCells(); ++i)
        // {
        //     auto c = ps->GetCell(i);
        //     if (c->GetCellDimension()==2)
        //     {
        //         c->Triangulate(
        //         triangle->GetPoints()->GetPoint(0, p0);
        //         triangle->GetPoints()->GetPoint(1, p1);
        //         triangle->GetPoints()->GetPoint(2, p2);
        //         double area = vtkTriangle::TriangleArea(p0, p1, p2);
        //     }
        // }
    }
};



int main(int argc, char *argv[])
{
    boost::filesystem::path datadir(argv[1]);

    auto r = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
    r->SetFileName( (datadir/"outlet_cyl.vtm").string().c_str() );
    r->Update();

    auto pf = vtkSmartPointer<vtkCompositeDataToUnstructuredGridFilter>::New();
    pf->AddInputConnection(r->GetOutputPort());
    pf->Update();

    // pf->GetOutput()->Print(std::cout);
    OptimalViews ovs(pf->GetOutput());

    return 0;
}
