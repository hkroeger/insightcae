#include <iostream>

#include "occtools.h"

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

    catch (const std::exception& e)
    {
      std::cerr<<e.what()<<std::endl;
      return -1;
    }

    return 0;
}
