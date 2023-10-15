#include "base/linearalgebra.h"
#include "base/resultset.h"
#include "base/resultelements/contourchart.h"
#include "base/vtktools.h"
#include "base/cacheableentity.h"

#include "vtkSTLReader.h"

using namespace insight;

int main(int argc, char* argn[])
{
    try
    {
        insight::assertion(argc==2, "expected data directory as a single argument!");

        boost::filesystem::path basedir(argn[1]);

        auto read = [](const boost::filesystem::path& f)
        {
            std::cout<<">> requesting "<<f.string()<<std::endl;
            auto obj =
                Cached<vtkSmartPointer<vtkPolyData>, const boost::filesystem::path&>(
                    [](const boost::filesystem::path& fp) -> vtkSmartPointer<vtkPolyData>
                    {
                        auto surface = vtkSmartPointer<vtkSTLReader>::New();
                        surface->SetFileName(fp.string().c_str());
                        surface->Update();
                        return surface->GetOutput();
                    }
                ) (f);
        };

        read(basedir/"cylinder.stl");
        read(basedir/"cylinder.stl");
        read(basedir/"sphere.stl");
        read(basedir/"cylinder.stl");
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
