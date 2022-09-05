
#include "base/exception.h"
#include "base/spatialtransformation.h"
#include "occtools.h"

#include "vtkTransform.h"

using namespace insight;

int main(int /*argc*/, char*/*argv*/[])
{
  try
  {
        {
            SpatialTransformation t(vec3(1,2,3), vec3(0,0,0));
            std::cout<<t<<std::endl;
            insight::assertion(
                        arma::norm( t(vec3(1,1,1))-vec3(2,3,4), 2)< SMALL,
                        "error in translation" );
        }
        {
            SpatialTransformation t(vec3(0,0,0), vec3(0,0,90));
            std::cout<<t<<std::endl;
            insight::assertion(
                        arma::norm( t(vec3(1,0,0))-vec3(0,1,0), 2)< SMALL,
                        "error in rotation" );
        }
        {
            SpatialTransformation t(vec3(0,0,0), vec3(0,0,0), 2.);
            std::cout<<t<<std::endl;
            insight::assertion(
                        arma::norm( t(vec3(1,0,0))-vec3(2,0,0), 2)< SMALL,
                        "error in scaling" );
        }

        {
            SpatialTransformation t(vec3(1,1,1), vec3(0,0,90));
            std::cout<<t<<std::endl;
            insight::assertion(
                        arma::norm( t.trsfVec(vec3(1,0,0))-vec3(0,1,0), 2)< SMALL,
                        "error in rotation w translation" );
        }

        {
            SpatialTransformation t1(vec3(1,2,3), vec3(4,5,6), 3);
            SpatialTransformation t2(vec3(7,8,9), vec3(11,13,17), 5);
            auto t3 = t1.appended(t2);

            std::cout<<t1<<std::endl;
            std::cout<<t2<<std::endl;
            std::cout<<t3<<std::endl;

            auto p=vec3(15, 17, 29);
            std::cout<<t2.trsfPt(t1.trsfPt(p)).t()<<std::endl;
            std::cout<<t3.trsfPt(p).t()<<std::endl;

            insight::assertion(
                        arma::norm(
                            t2.trsfPt(t1.trsfPt(p))
                            -
                            t3.trsfPt(p)
                            , 2)< SMALL,
                        "error in subsequent combined transformation" );
        }


        {
            arma::mat p=vec3(1,2,3);

            vtkNew<vtkTransform> t;
            t->Translate(3,4,5);
            t->RotateZ(10);
            t->RotateX(20);
            t->RotateY(30);
            t->Scale(1.2,1.2,1.2);

            arma::mat pdash_t=vec3Zero();
            t->TransformPoint(p.memptr(), pdash_t.memptr());

            SpatialTransformation t1(t);
            arma::mat pdash_t1 = t1.trsfPt(p);

            std::cout<<pdash_t.t()<<" <=> "<<pdash_t1.t()<<std::endl;
            insight::assertion(
                        arma::norm(pdash_t-pdash_t1)<SMALL,
                        "error in conversion from vtk");

            auto t2 = t1.toVTKTransform();
            arma::mat pdash_t2=vec3Zero();
            t2->TransformPoint(p.memptr(), pdash_t2.memptr());

            std::cout<<pdash_t.t()<<" <=> "<<pdash_t2.t()<<std::endl;
            insight::assertion(
                        arma::norm(pdash_t-pdash_t2)<SMALL,
                        "error in back conversion from SpatialTransform to vtk");
        }

   }
  catch (const std::exception& e)
  {
    std::cerr<<e.what()<<std::endl;
    return -1;
  }

  return 0;
}
