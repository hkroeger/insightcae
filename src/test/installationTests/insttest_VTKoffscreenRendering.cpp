
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/vtkrendering.h"

#include "vtkCubeSource.h"
#include "vtkPolyDataMapper.h"

using namespace insight;
using namespace std;
namespace fs=boost::filesystem;

int main()
{
  VTKOffscreenScene scene;
  auto box = vtkSmartPointer<vtkCubeSource>::New();

  auto camera = scene.activeCamera();
  camera->ParallelProjectionOn();
  camera->SetFocalPoint( toArray(vec3(0,0,0)) );
  camera->SetViewUp( toArray(vec3(0,0,1)) );
  camera->SetPosition( toArray(vec3(2,3,4)) );
  scene.setParallelScale(std::pair<double,double>(5, 5));

  box->Update();
  scene.addData<vtkPolyDataMapper>(box->GetOutput(), vec3(0.1,0.1,0.1));


  scene.setParallelScale(1);
  scene.fitAll();

  auto img = fs::unique_path(fs::temp_directory_path()/"%%%%%.png");
  cout<<"writing "<<img.string()<<endl;

  scene.exportImage(img);

  if (!fs::exists(img))
  {
    cerr<<"target file not created!"<<endl;
    return -1;
  }
  else
  {
    fs::remove(img);
  }

  return 0;
}
