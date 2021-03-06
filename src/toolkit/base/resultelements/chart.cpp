#include "chart.h"

#include "base/tools.h"

#include "gnuplot-iostream.h"

#include "cpp/poppler-document.h"
#include "cpp/poppler-page.h"
#include "cpp/poppler-page-renderer.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;



namespace insight {




PlotCurve::PlotCurve()
{
}

PlotCurve::PlotCurve(const PlotCurve& o)
: xy_(o.xy_), plotcmd_(o.plotcmd_), plaintextlabel_(o.plaintextlabel_)
{}


PlotCurve::PlotCurve(const std::string& plaintextlabel, const char* plotcmd)
: plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{
}

PlotCurve::PlotCurve(const std::vector<double>& x, const std::vector<double>& y, const std::string& plaintextlabel, const std::string& plotcmd)
: plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{
  if (x.size()!=y.size())
  {
      throw insight::Exception
      (
        boost::str(boost::format("plot curve %s: number of point x (%d) != number of points y (%d)!")
          % plaintextlabel_ % x.size() % y.size() )
      );
  }

  xy_ = join_rows( arma::mat(x.data(), x.size(), 1), arma::mat(y.data(), y.size(), 1) );
}

PlotCurve::PlotCurve(const arma::mat& x, const arma::mat& y, const std::string& plaintextlabel, const std::string& plotcmd)
: plotcmd_(plotcmd),
  plaintextlabel_(plaintextlabel)
{
  if (x.n_rows!=y.n_rows)
  {
      throw insight::Exception
      (
        boost::str(boost::format("plot curve %s: number of point x (%d) != number of points y (%d)!")
          % plaintextlabel_ % x.n_rows % y.n_rows )
      );
  }
  xy_ = join_rows(x, y);
}

PlotCurve::PlotCurve ( const arma::mat& xrange, double y, const std::string& plaintextlabel, const std::string& plotcmd )
: plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{
    xy_
     << arma::as_scalar(arma::min(xrange)) << y << arma::endr
     << arma::as_scalar(arma::max(xrange)) << y << arma::endr
     ;
}


PlotCurve::PlotCurve(const arma::mat& xy, const std::string& plaintextlabel, const std::string& plotcmd)
: xy_(xy), plotcmd_(plotcmd), plaintextlabel_(plaintextlabel)
{}

void PlotCurve::sort()
{
  xy_=sortedByCol(xy_, 0);
}


std::string PlotCurve::title() const
{
  boost::regex re(".*t *'(.*)'.*");
  boost::smatch str_matches;
  if (boost::regex_match(plotcmd_, str_matches, re))
  {
//     std::cout<<" <> "<<str_matches[1]<<std::endl;
    return str_matches[1];
  }
  else return "";
}

const arma::mat& PlotCurve::xy() const
{
    return xy_;
}
const std::string& PlotCurve::plaintextlabel() const
{
    return plaintextlabel_;
}






PlotCurveList::PlotCurveList()
 : std::vector<PlotCurve>()
{}


PlotCurveList::PlotCurveList(size_t n)
 : std::vector<PlotCurve>(n)
{}


PlotCurveList::PlotCurveList(std::initializer_list<PlotCurve> il)
 : std::vector<PlotCurve>(il)
{}


PlotCurveList::PlotCurveList(const_iterator begin, const_iterator end)
 : std::vector<PlotCurve>(begin, end)
{}







defineType(Chart);
addToFactoryTable(ResultElement, Chart);



Chart::Chart(const std::string& shortdesc, const std::string& longdesc, const std::string& unit)
: ResultElement(shortdesc, longdesc, unit)
{
}



Chart::Chart
(
  const std::string& xlabel,
  const std::string& ylabel,
  const PlotCurveList& plc,
  const std::string& shortDesc, const std::string& longDesc,
  const std::string& addinit
)
: ResultElement(shortDesc, longDesc, ""),
  xlabel_(xlabel),
  ylabel_(ylabel),
  plc_(plc),
  addinit_(addinit)
{
}


void Chart::gnuplotCommand(gnuplotio::Gnuplot& gp) const
{
 gp<<addinit_<<";";
 gp<<"set xlabel '"<<xlabel_<<"'; set ylabel '"<<ylabel_<<"'; set grid; ";
 if ( plc_.size() >0 )
 {
  gp<<"plot ";
  bool is_first=true;

  if (plc_.include_zero)
  {
   gp<<"0 not lt -1";
   is_first=false;
  }

  for ( const PlotCurve& pc: plc_ )
  {
   if ( !pc.plotcmd_.empty() )
   {
    if (!is_first) { gp << ","; is_first=false; }
    if ( pc.xy_.n_rows>0 )
    {
     gp<<"'-' "<<pc.plotcmd_;
    } else
    {
     gp<<pc.plotcmd_;
    }
   }
  }

  gp<<endl;

  for ( const PlotCurve& pc: plc_ )
  {
   if ( pc.xy_.n_rows>0 )
   {
    gp.send1d ( pc.xy_ );
   }
  }

 }
}


void Chart::generatePlotImage( const path& imagepath ) const
{
  CurrentExceptionContext ex("rendering chart into image "+imagepath.string());

  std::string bn ( imagepath.filename().stem().string() );

  bool keep=false;
  if (getenv("INSIGHT_KEEP_TEMP_DIRS"))
    keep=true;
  CaseDirectory tmp ( keep, bn+"-generate" );

  {
    CurrentExceptionContext ex("executing gnuplot");

    Gnuplot gp;

    //gp<<"set terminal pngcairo; set termoption dash;";
    gp<<"set terminal epslatex standalone color dash linewidth 3 header \"\\\\usepackage{graphicx}\\n\\\\usepackage{epstopdf}\";";
    gp<<"set output '"+bn+".tex';";
    //     gp<<"set output '"<<absolute(imagepath).string()<<"';";
    /*
            gp<<"set linetype  1 lc rgb '#0000FF' lw 1;"
                "set linetype  2 lc rgb '#8A2BE2' lw 1;"
                "set linetype  3 lc rgb '#A52A2A' lw 1;"
                "set linetype  4 lc rgb '#E9967A' lw 1;"
                "set linetype  5 lc rgb '#5F9EA0' lw 1;"
                "set linetype  6 lc rgb '#006400' lw 1;"
                "set linetype  7 lc rgb '#8B008B' lw 1;"
                "set linetype  8 lc rgb '#696969' lw 1;"
                "set linetype  9 lc rgb '#DAA520' lw 1;"
                "set linetype cycle  9;";
        */

    gnuplotCommand(gp);
  }

  std::system (
        (
          "mv "+bn+".tex "+ ( tmp/ ( bn+".tex" ) ).string()+"; "
          "mv "+bn+"-inc.eps "+ ( tmp/ ( bn+"-inc.eps" ) ).string()+"; "
          "cd "+tmp.string()+"; "
          "pdflatex -interaction=batchmode -shell-escape "+bn+".tex; "
          //"convert -density 600 "+bn+".pdf "+absolute ( imagepath ).string()
          ).c_str() );

  std::shared_ptr<poppler::document> doc( poppler::document::load_from_file( (tmp/(bn+".pdf")).string() ) );
  if (!doc) {
    throw insight::Exception("loading error");
  }
  if (doc->is_locked()) {
    throw insight::Exception("pdflatex produced encrypted document");
  }
  if (doc->pages()!=1)
    throw insight::Exception(str(format("expected one single page in chart PDF, got %d!")%doc->pages()));

  std::shared_ptr<poppler::page> page(doc->create_page(0));
  if (!page) {
    throw insight::Exception("could not extract page from PDF document");
  }
  poppler::page_renderer pr;
  pr.set_render_hint(poppler::page_renderer::antialiasing, true);
  pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);

  poppler::image img = pr.render_page(page.get(), 600, 600);
  if (!img.is_valid()) {
    throw insight::Exception("rendering failed");
  }

  if (!img.save( absolute(imagepath).string(), "png", 600)) {
    throw insight::Exception("saving to file failed");
  }
}


void Chart::writeLatexHeaderCode(std::ostream& f) const
{
  f<<"\\usepackage{graphicx}\n";
  f<<"\\usepackage{placeins}\n";
}


void Chart::writeLatexCode ( std::ostream& f, const std::string& name, int , const boost::filesystem::path& outputfilepath ) const
{
    path chart_file=cleanLatexImageFileName ( outputfilepath/ ( name+".png" ) ).string();

    generatePlotImage ( chart_file );

    //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
    f<<
     "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{" << make_relative ( outputfilepath, chart_file ).c_str() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
     "\\FloatBarrier";
}


void Chart::exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const
{
    int curveID=0;
    for ( const PlotCurve& pc: plc_ ) {
        std::string suf=pc.plaintextlabel();
        replace_all ( suf, "/", "_" );
        if ( suf=="" ) {
            suf=str ( format ( "curve%d" ) %curveID );
        }

        boost::filesystem::path fname ( outputdirectory/ ( name+"__"+suf+".xy" ) );

        std::ofstream f ( fname.c_str() );
        pc.xy_.save ( fname.string(), arma::raw_ascii );
        curveID++;
    }
}


rapidxml::xml_node<>* Chart::appendToNode
(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node
) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "xlabel",
                                  doc.allocate_string ( xlabel_.c_str() )
                              ) );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "ylabel",
                                  doc.allocate_string ( ylabel_.c_str() )
                              ) );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "addinit",
                                  doc.allocate_string ( addinit_.c_str() )
                              ) );

    for ( const PlotCurve& pc: plc_ ) {
        xml_node<> *pcnode = doc.allocate_node
                             (
                                 node_element,
                                 "PlotCurve"
                             );
        child->append_node ( pcnode );

        pcnode->append_attribute
        (
            doc.allocate_attribute
            (
                "plaintextlabel",
                doc.allocate_string ( pc.plaintextlabel().c_str() )
            )
        );

        pcnode->append_attribute
        (
            doc.allocate_attribute
            (
                "plotcmd",
                doc.allocate_string ( pc.plotcmd_.c_str() )
            )
        );

        writeMatToXMLNode ( pc.xy_, doc, *pcnode );
    }

    return child;
}

void Chart::readFromNode(const string &name, rapidxml::xml_document<> &doc, rapidxml::xml_node<> &node)
{
  readBaseAttributesFromNode(name, doc, node);
  xlabel_=node.first_attribute("xlabel")->value();
  ylabel_=node.first_attribute("ylabel")->value();
  addinit_=node.first_attribute("addinit")->value();
  for (xml_node<> *e = node.first_node(); e; e = e->next_sibling())
  {
    std::string plaintextlabel=e->first_attribute("plaintextlabel")->value();
    std::string plotcmd=e->first_attribute("plotcmd")->value();
    std::string value_str=e->value();
    std::istringstream iss(value_str);
    arma::mat xy;
    xy.load(iss, arma::raw_ascii);
    plc_.push_back(PlotCurve(xy, plaintextlabel, plotcmd));
  }
}




ResultElementPtr Chart::clone() const
{
    ResultElementPtr res ( new Chart ( xlabel_, ylabel_, plc_, shortDescription().simpleLatex(), longDescription().simpleLatex(), addinit_ ) );
    res->setOrder ( order() );
    return res;
}








insight::ResultElement& addPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const boost::filesystem::path& ,
    const std::string& resultelementname,
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotCurveList& plc,
    const std::string& shortDescription,
    const std::string& addinit,
    const std::string& watermarktext
)
{
    std::string precmd=addinit+";";
    if ( watermarktext!="" ) {
        precmd+=
          "set label "
          "'"+SimpleLatex( watermarktext ).toLaTeX()+"'"
          " center at screen 0.5, 0.5 tc rgb\"#cccccc\" rotate by 30 font \",24\";"
          ;
    }

    return results->insert ( resultelementname,
                             new Chart
                             (
                                 xlabel, ylabel, plc,
                                 shortDescription, "",
                                 precmd
                             ) );
}


} // namespace insight
