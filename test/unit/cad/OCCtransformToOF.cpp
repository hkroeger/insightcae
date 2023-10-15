#include <iostream>

#include "vtkTransform.h"

#include "base/linearalgebra.h"
#include "base/spatialtransformation.h"
#include "base/exception.h"

#include "occtools.h"

using namespace insight;

std::ostream& operator<<(std::ostream& os, const insight::cad::OCCtransformToOF& tr)
{
    os<<"1) translate by "<<tr.translate().t();
    os<<"2) rotate (roll>pitch>yaw) by "<<tr.rollPitchYaw().t();
    os<<"3) scale by "<<tr.scale()<<std::endl;
    return os;
}

int main(int, char*[])
{
    try
    {
        {
            gp_Trsf t1; t1.SetTranslation(gp_Vec(1., 0, 1.));
            gp_Trsf t2; t2.SetRotation(gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,0,1)), 0.25*M_PI);
            gp_Trsf t2q; t2q.SetRotation(gp_Ax1(gp_Pnt(1,0,0), gp_Dir(0,1,0)), 0.25*M_PI);
            gp_Trsf t3; t3.SetScale(gp::Origin(), 1000);

            std::cout
                   <<"pure translation:\n"
                   <<insight::cad::OCCtransformToOF(t1)<<std::endl;

            std::cout
                   <<"pure rotation:\n"
                   <<insight::cad::OCCtransformToOF(t2)<<std::endl;

            std::cout
                   <<"translation, then rotation:\n"
                   <<insight::cad::OCCtransformToOF(t2*t1)<<std::endl;

            std::cout
                   <<"translation, then rotation, then scale:\n"
                   <<insight::cad::OCCtransformToOF(t3*t2*t1)<<std::endl;

            std::cout
                   <<"translation, then rotation (off center), then scale:\n"
                   <<insight::cad::OCCtransformToOF(t3*t2q*t1)<<std::endl;
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

            auto t2 = cad::OFtransformToOCC(insight::SpatialTransformation(t));
            arma::mat pdash_t2 = Vector(to_Pnt(p).Transformed(t2));
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
