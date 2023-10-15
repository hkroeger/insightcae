#include "base/exception.h"
#include "base/units.h"

using namespace insight;

int main(int /*argc*/, char*/*argv*/[])
{
    try
    {

        si::length unit1( si::meter );
        si::Length unit2( 1*si::millimeter );
        si::Length unit3( 5*si::millimeter );

        std::cout<<"v="<< toValue(1.5*si::meter, si::meter) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, si::millimeter) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, 5*si::millimeter) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, unit1) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, unit2) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, unit3) <<std::endl;
        std::cout<<"v="<< toValue(vec3X(1.5)*si::meter, unit3) <<std::endl;

    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
