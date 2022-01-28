#include "latexgnuplotrenderer.h"

#include "base/cppextensions.h"

namespace insight
{

std::unique_ptr<gnuplotio::Gnuplot> make_Gnuplot()
{
    std::string pname = ExternalPrograms::path("gnuplot").string();
    boost::replace_all(pname, " ", "\" \"");
    std::string cmd =
            pname
            +" -persist"
#ifdef WIN32
            +" 2> NUL"
#endif
            ;
//    insight::dbg()<<"executing: "<<cmd<<std::endl;
    return std::make_unique<gnuplotio::Gnuplot>( static_cast<const std::string&>(cmd) );
}

}
