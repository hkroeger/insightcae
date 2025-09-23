
#include "base/exception.h"

#include "cadfeature.h"
#include "cadmodel.h"
#include "parser.h"


using namespace insight;
using namespace insight::cad;

int main(int, char*argv[])
{
    try
    {

        std::string scr=
            "H=10;"
            "t_stiff=1.;"
            "ab_1:Cylinder(O, H*EX, 1);"
            "ab_2=Cylinder(O, H*EX, 2);"
            "ab_3?=Cylinder(O, H*EX, 3);"
            "xx=-7;"
            "xxx=-xx;"
            "p_O=ab_1@p1;"
            "p_O2=-p_O;"
            "ab1Da=ab_1$Da;"
            "delta=ab_1$Da-ab_2$Da;"
            "y=(0.5*ab_1$Da+1.5*t_stiff)*EY +(3.4+t_stiff)*EZ;"
            "endofs=(-EZ-EY)*0.5*ab_2$Da;"
            "side_ofs=0.5*(-ab_2$Da)+ab_3$Da;"
            "zz=(H-t_stiff)*-EX;"
            "x1=-0.7*ab_1$Da*EX +1.1*ab_2$Da*EY;"
            "x2=-ab_1$Da*EY;"
            ;

        auto m = std::make_shared<cad::Model>();

        insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir;
        int failloc=-1;

        if (!parseISCADModel(
            scr, m.get(), &failloc, &syn_elem_dir) )
        {
            throw insight::Exception("Parser failed at \"%s\"", scr.substr(failloc).c_str());
        }

        std::cout << *m << std::endl;
    }
    catch (insight::Exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }
    return 0;
}
