
#include "base/exception.h"
#include "base/spatialtransformation.h"
#include "base/boost_include.h"
#include "boost/format.hpp"

#include "vtkTransform.h"

using namespace insight;

int main(int /*argc*/, char*/*argv*/[])
{
  try
  {
        int n=5;
        int nWrong=0, nTried=0;
        for (int i=0; i<n; i++)
        {
            for (int j=0; j<n; j++)
            {
                for (int k=0; k<n; k++)
                {
                    nTried++;
                    bool wrong=false;

                    arma::mat rpy=vec3(
                        -180.+(i/double(n))*360.,
                        -180.+(j/double(n))*360.,
                        -180.+(k/double(n))*360.
                        );

                    arma::mat disp=vec3(
                        -100.+(i/double(n))*200.,
                        -100.+(j/double(n))*200.,
                        -100.+(k/double(n))*200.
                        );
                    SpatialTransformation t(disp, rpy);

                    arma::mat rpyp=t.rollPitchYaw();

                    SpatialTransformation t2(t.translate(), rpyp);

                    if (t!=t2)
                    {
                        wrong=true;
                        std::cout
                            <<boost::str(boost::format("wrong roll/pitch/yaw conversion for (%g, %g, %g): back conversion yields (%g, %g, %g)")
                                    %rpy(0)%rpy(1)%rpy(2)
                                    %rpyp(0)%rpyp(1)%rpyp(2))
                                  <<std::endl;
                    }

                    arma::mat pt=vec3(
                        -200.+(i/double(n))*400.,
                        -200.+(j/double(n))*400.,
                        -200.+(k/double(n))*400.
                        );

                    arma::mat pdash = t.trsfPt(pt);
                    arma::mat pdashdash = t.inverted().trsfPt(pdash);

                    if (arma::norm(pdashdash-pt,2)>SMALL)
                    {
                        wrong=true;
                        std::cout
                            <<boost::str(boost::format("forth and back transformation did not yield equality: pt=(%g, %g, %g), pdash=(%g, %g, %g), pdashdash=(%g, %g, %g)")
                                          %pt(0)%pt(1)%pt(2)
                                          %pdash(0)%pdash(1)%pdash(2)
                                          %pdashdash(0)%pdashdash(1)%pdashdash(2))
                            <<std::endl;
                    }

                    if (wrong) nWrong++;

                }
            }
        }
        insight::assertion(
            nWrong==0,
            "%d of %d roll/pitch/yaw conversions failed", nWrong, nTried);

        {
            SpatialTransformation t(vec3(1,2,3), vec3(0,0,0));
            std::cout<<t<<std::endl;
            insight::assertion(
                        arma::norm( t(vec3(1,1,1))-vec3(2,3,4), 2)< SMALL,
                        "error in translation" );
        }
        {
            SpatialTransformation t(vec3(0,0,0), vec3(0,0,0), 2.);
            std::cout<<t<<std::endl;
            insight::assertion(
                arma::norm( t(vec3(1,0,0))-vec3(2,0,0), 2)< SMALL,
                "error in scaling" );
        }
        {
            SpatialTransformation t(vec3(0,0,0), vec3(0,0,90));
            std::cout
                <<t
                <<t(vec3(1,0,0)).t()
                <<t.R()
                <<std::endl;
            insight::assertion(
                        arma::norm( t(vec3(1,0,0))-vec3(0,1,0), 2)< SMALL,
                        "error in rotation" );
        }

        {
            SpatialTransformation t(vec3(1,1,1), vec3(0,0,90));
            std::cout<<t<<std::endl;
            insight::assertion(
                        arma::norm( t.trsfVec(vec3(1,0,0))-vec3(0,1,0), 2)< SMALL,
                        "error in rotation w translation" );
        }

        {
            auto trans=vec3(1,2,3);
            CoordinateSystem cs2(trans, vec3(0,1,0), vec3(0, 0, 1));
            auto t1=cs2.localToGlobal();
            auto t2=t1.inverted();

            auto org = vec3(1,0,0);
            auto org2 = t2(t1(org)); // transform forth and back

            arma::mat interm=t1(org).t();
            std::cout << "inverse forth and back ("<<trans.t()<<") :"
                      << org.t() << ">>"
                      << interm << ">>"
                      << org2.t();
            insight::assertion(
                fabs( (interm(0)-trans(0)))<SMALL,
                "unexpected x component after trsf"
                );
            insight::assertion(
                fabs( (interm(1)-trans(1)) - org(0))<SMALL,
                "unexpected y component after trsf"
                );
            insight::assertion(
                fabs( (interm(2)-trans(2)))<SMALL,
                "unexpected z component after trsf"
                );
            insight::assertion(
                arma::norm(org-org2,2)<SMALL,
                "inverse transform does not transform back");
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
