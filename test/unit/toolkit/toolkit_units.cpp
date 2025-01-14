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
        std::cout<<"v="<< toValue(1.5*si::meter, 5.*si::millimeter) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, unit1) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, unit2) <<std::endl;
        std::cout<<"v="<< toValue(1.5*si::meter, unit3) <<std::endl;
        std::cout<<"v="<< toValue(vec3X(1.5)*si::meter, unit3) <<std::endl;

        insight::assertion(
            toValue(1.5*si::meter, si::meter)==1.5,
            "identity conversion failed!");

        insight::assertion(
            toValue(1.5*si::meter, unit1)==1.5,
            "length unit1 failed!");

        insight::assertion(
            toValue(1.5*si::meter, unit2)==1500.,
            "length unit2 failed!");

        insight::assertion(
            toValue(1.5*si::meter, unit3)==300.,
            "length unit3 failed!");

        insight::assertion(
            arma::norm(toValue(vec3X(1.5)*si::meter, unit3),2)-300.==0.,
            "vector length failed!");


        auto t1=20.*si::degC;
        auto t2=300.*si::degK;

        si::Temperature t(t1);

        std::cout
            <<t1<<" "<<t1.value()<<std::endl
            <<t2<<" "<<t2.value()<<std::endl
            <<t<<" "<<decltype(si::degK)(t1).value()<<std::endl
            << si::toValue(t1, si::degC)<<"°C"<<std::endl
            << si::toValue(t1, si::degK)<<"°K"<<std::endl
            << si::toValue(t2, si::degC)<<"°C"<<std::endl
            << si::toValue(t2, si::degK)<<"°K"<<std::endl;

        insight::assertion(
            fabs(si::toValue(t1, si::degC)-20.)<SMALL,
            "identity degC failed!"
            );
        insight::assertion(
            fabs(si::toValue(t1, si::degK)-293.15)<LSMALL,
            "degC=>degK failed!"
            );
        insight::assertion(
            fabs(si::toValue(t2, si::degC)-26.85)<LSMALL,
            "degK=>degC failed!"
            );
        insight::assertion(
            fabs(si::toValue(t2, si::degK)-300.)<SMALL,
            "identity degK failed!"
            );

        auto rho=555.*cgs::gram/pow<3>(cgs::centimeter);
        std::cout<<toValue( rho, si::kilogram/pow<3>(si::meter) )<<std::endl;
        insight::assertion(
            fabs(toValue( rho, si::kilogram/pow<3>(si::meter) )-555000.)<LSMALL,
            "density conversion failed!");

        auto h=64. * si::kilojoule_per_hour_squaremeter_kelvin;
        std::cout<<toValue( h, si::joule/si::second/si::square_meter/si::kelvin )<<std::endl;
        insight::assertion(
            fabs(toValue( h, si::joule/si::second/si::square_meter/si::kelvin )-17.777778)<LSMALL,
            "heat transfer coefficient conversion failed!");
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
