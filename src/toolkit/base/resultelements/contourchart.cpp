#include "contourchart.h"

#include "gnuplot-iostream.h"
#include "base/resultelements/image.h"
#include "base/resultelements/gnuplotchartrenderer.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;



namespace insight {



PlotField::PlotField()
{
}


PlotField::PlotField ( const arma::mat& xy, const std::string& plotcmd )
    : xy_ ( xy ), plotcmd_ ( plotcmd )
{}




void addContourPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& workdir,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotFieldList& plc,
    const std::string& shortDescription,
    const std::string& addinit
)
{
    std::string chart_file_name= ( workdir/ ( resultelementname+".png" ) ).string();
    //std::string chart_file_name_i=(workdir/(resultelementname+".ps")).string();

    {
        auto gp=make_Gnuplot();

        //gp<<"set terminal postscript color;";
        //gp<<"set output '"<<chart_file_name_i<<"';";
        *gp<<"set terminal pngcairo; set termoption dash;";
        *gp<<"set output '"<<chart_file_name<<"';";

        *gp<<addinit<<";";
        *gp<<"set xlabel '"<<xlabel<<"'; set ylabel '"<<ylabel<<"'; set grid; ";
        *gp<<"splot ";
        for ( const PlotCurve& pc: plc ) {
            *gp<<"'-' "<<pc.plotcmd_;
        }
        *gp<<endl;
        for ( const PlotCurve& pc: plc ) {
            gp->send ( pc.xy_ );
        }
    }
    /*
     std::string cmd="ps2pdf "+chart_file_name_i+" "+chart_file_name;
     int ret=::system(cmd.c_str());
     if (ret || !exists(chart_file_name))
      throw insight::Exception("Conversion from postscript chart to pdf failed! Command was:\n"+cmd);
     else
      remove(chart_file_name_i);
      */
    results->insert ( resultelementname,
                      std::unique_ptr<Image> ( new Image
                              (
                                  workdir, chart_file_name,
                                  shortDescription, ""
                              ) ) );
}



} // namespace insight
