
#include "base/exception.h"
#include "base/casedirectory.h"
#include "base/spatialtransformation.h"

#include "openfoam/openfoamcase.h"


#include "vtkCubeSource.h"
#include "vtkTriangleFilter.h"
#include "vtkSTLWriter.h"
#include "vtkSTLReader.h"
#include "vtkCleanPolyData.h"

using namespace insight;


int main(int argc, char* argv[])
{
    try
    {
        CaseDirectory d(false);

        boost::filesystem::path input_fname( d/"object_input.stl" );
        boost::filesystem::path OFtrsf_fname( d/"object_trsf_of.stlb" );
        boost::filesystem::path IStrsf_fname( d/"object_trsf_is.stl" );

        auto b=vtkSmartPointer<vtkCubeSource>::New();
        b->SetXLength(2.);
        b->SetYLength(3.);
        b->SetZLength(7.);
        b->SetCenter(8., 19., 27.);

        auto tri = vtkSmartPointer<vtkTriangleFilter>::New();
        tri->SetInputConnection(b->GetOutputPort());

        auto clean = vtkSmartPointer<vtkCleanPolyData>::New();
        clean->SetInputConnection(tri->GetOutputPort(0));

        auto untrsf = clean;

        {
            auto savestl = vtkSmartPointer<vtkSTLWriter>::New();
            savestl->SetInputConnection(untrsf->GetOutputPort(0));
            savestl->SetFileTypeToBinary();
            savestl->SetFileName( input_fname.string().c_str() );
            savestl->Update();
        }

        auto scale = 7.;
        auto translate = vec3(1., 18., 27.);
        auto rollPitchYaw = vec3(79., 37., 11.);//deg

        OpenFOAMCase ofc(OFEs::get(argv[1]));

        ofc.executeCommand(
           d, "surfaceTransformPoints",
           {
               absolute(input_fname).string(),
               absolute(OFtrsf_fname).string(),

               "-scale", OFDictData::toString(OFDictData::vector3(vec3(scale, scale, scale))),
               "-translate", OFDictData::toString(OFDictData::vector3(translate)),
               "-rollPitchYaw", OFDictData::toString(OFDictData::vector3(rollPitchYaw))
           }
        );

        SpatialTransformation trsf(translate, rollPitchYaw, scale);

        arma::mat reconAngles = trsf.rollPitchYaw();
        std::cout<<"recon = "<<reconAngles<<std::endl;
        insight::assertion(
            arma::norm(reconAngles-rollPitchYaw,2)<SMALL,
            "rotation angles are not properly reconstructed from rotation matrix");

        auto trsf_is = trsf.apply_VTK_Transform(untrsf);

        {
            auto savestl = vtkSmartPointer<vtkSTLWriter>::New();
            savestl->SetInputConnection(trsf_is->GetOutputPort(0));
            savestl->SetFileTypeToBinary();
            savestl->SetFileName( IStrsf_fname.string().c_str() );
            savestl->Update();
        }

        {
            auto trsf_of = vtkSmartPointer<vtkSTLReader>::New();
            trsf_of->SetFileName( OFtrsf_fname.string().c_str() );
            trsf_of->Update();

            auto o1=trsf_is->GetOutput();
            auto o2=trsf_of->GetOutput();

            vtkIdType
                np1=o1->GetNumberOfPoints(),
                np2=o2->GetNumberOfPoints();

            insight::assertion(
                np1==np2,
                "expected equal number of points (%d != %d)", np1, np2 );

            std::vector<double> diffs(size_t(o1->GetNumberOfPoints()), DBL_MAX);
            double diffnorm=0;
            for (vtkIdType i1=0; i1<np1; ++i1)
            {
                for (vtkIdType i2=0; i2<np2; ++i2)
                {
                    auto p1=vec3FromComponents(o1->GetPoint(i1));
                    auto p2=vec3FromComponents(o2->GetPoint(i2));
                    dbg(Loops)<<p1.t()<<p2.t()<<std::endl;
                    diffs[i1]=std::min(diffs[i1], arma::norm(p2-p1,2));
                }

                dbg(DetailedBusiness)<<diffs[i1]<<std::endl;

                diffnorm+=pow(diffs[i1],2);
            }
            insight::assertion(
                diffnorm<SMALL,
                "there is a difference between point location (%g)", diffnorm );


        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }
    return 0;
}
