#include "contourchart.h"

#include "gnuplot-iostream.h"
#include "base/rapidxml.h"
#include "base/resultelements/image.h"
#include "base/resultelements/latexgnuplotrenderer.h"

#include "base/parameters/doublerangeparameter.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;



namespace insight {




PlotField::PlotField()
{}

PlotField::PlotField ( const arma::mat& x,
                       const arma::mat& y,
                       const arma::mat& data  )
    : x_(x), y_(y), data_(data)
{
    insight::assertion(
                x_.n_elem==data_.n_rows,
                str(format("invalid size of data table: number of elements in x coordinate array (%d) must be equal to number of rows in data table (%d)")
                % x_.n_elem % data_.n_rows) );
    insight::assertion(
                y_.n_elem==data_.n_cols,
                str(format("invalid size of data table: number of elements in y coordinate array (%d) must be equal to number of cols in data table (%d)")
                % y_.n_elem % data_.n_cols) );
}


double GnuplotPolarContourChartRenderer::canvasSizeRatio() const
{
    return 0.7/0.9;
}

GnuplotPolarContourChartRenderer::GnuplotPolarContourChartRenderer(const PolarContourChart &pcc)
    : pcc_(pcc)
{
    for (int i=0; i<pcc_.x_.n_elem; ++i)
    {
          for (int j=0; j<pcc_.y_.n_elem; ++j)
          {
              df_.stream()
                  << pcc_.x_[i] << " "
                  << pcc_.y_[j] << " "
                  << pcc_.data_(i,j) << endl;
          }
          df_.stream() << endl;
    }
    df_.closeStream();
}



void GnuplotPolarContourChartRenderer::gnuplotCommand(gnuplotio::Gnuplot &gp) const
{
//    gp<<"set tmargin at screen 0.1;"<<endl;

    gp<<"set lmargin at screen 0.05;"<<endl;
    gp<<"set bmargin at screen 0.1;"<<endl;
    gp<<"set tmargin at screen 0.9;"<<endl;
    gp<<"set rmargin at screen 0.75;"<<endl;
    gp<<"set colorbox user origin 0.77, 0.25;"<<endl;

    gp<<"r="<<pcc_.rmax_<<";"<<endl;

    gp<<"set pm3d at b noborder explicit;"<<endl;
    gp<<"set palette defined (0 \"white\", 0.01 \"yellow\", 0.05 \"orange\", 0.33 \"red\", 0.66 \"dark-red\", 1 \"dark-grey\" );"<<endl;
    gp<<"set view map;"<<endl;
    gp<<"set parametric;"<<endl;
    gp<<"set pm3d interpolate 0,0;"<<endl;
    gp<<"set isosamples 500,500;"<<endl;
    gp<<"set cblabel '"+pcc_.cblabel_+"';"<<endl;
    gp<<"set cbrange ["<<
         (pcc_.zClipMin_?lexical_cast<std::string>(*pcc_.zClipMin_):"")
         <<":"<<
         (pcc_.zClipMax_?lexical_cast<std::string>(*pcc_.zClipMax_):"")
         <<"];"<<endl;

    gp<<"unset key;"<<endl;
    gp<<"unset border;"<<endl;
    gp<<"unset xtics;"<<endl;
    gp<<"unset ytics;"<<endl;

    gp<<"set xrange[-r:r];"<<endl;
    gp<<"set yrange[-r:r];"<<endl;
//    gp<<"set size square;"<<endl;


    gp<<"set contour;"<<endl;
    if (pcc_.contourLines_.size()>0)
    {
      gp<<"set cntrparam levels discrete "
         <<toStringList(pcc_.contourLines_, "%g", ", ")<<";"<<endl;
    }

    gp<<"set multiplot;"<<endl;

    std::string use="u ($2*sin($1*pi/180)):(-$2*cos($1*pi/180)):3";
    gp<<"splot "
     "'"+df_.path().string()+"' "+use+" with pm3d, "
     "'' "+use+" with lines lw 2 nosurface, "
     "'' "+use+" with labels"<<endl;


    gp<<"set grid polar front;"<<endl;
    gp<<"set theta counterclockwise bottom;"<<endl;
    gp<<"set polar;"<<endl;
//    gp<<"set tmargin at screen 0.1;"<<endl;
//    gp<<"set rmargin at screen 0.75;"<<endl;
    gp<<"set rrange[0:r];"<<endl;
    gp<<"set rlabel '"<<pcc_.rlabel_<<"';"<<endl;

    for (const auto& a: {0., 45., 135., 180., 225., 270., 315.})
    {
        gp<<str(
                format("set label at polar (%g*pi/180.), (0.9*r) center \"$%3.0f^\\\\circ$\";")
                % a % a
                )<<endl;
    }

    gp<<"unset parametric;"<<endl;
    gp<<"plot NaN w l;"<<endl;

    gp<<"unset multiplot;"<<endl;
}



defineType(PolarContourChart);
addToFactoryTable(ResultElement, PolarContourChart);


PolarContourChart::PolarContourChart(
        const std::string &shortdesc,
        const std::string &longdesc,
        const std::string &unit)
    : ResultElement(shortdesc, longdesc, unit)
{}

PolarContourChart::PolarContourChart(
        const std::string &rlabel,
        const std::string &cblabel,
        double rmax,
        const PlotField &plf,
        const std::string &shortDesc,
        const std::string &longDesc,
        const std::vector<double>& contourLines,
        boost::optional<double> zClipMin,
        boost::optional<double> zClipMax )
    : ResultElement(shortDesc, longDesc, ""),
      PlotField(plf),
      rlabel_(rlabel),
      cblabel_(cblabel),
      rmax_(rmax),
      contourLines_(contourLines),
      zClipMin_(zClipMin),
      zClipMax_(zClipMax)
{}






void PolarContourChart::generatePlotImage(const boost::filesystem::path &imagepath) const
{
    GnuplotPolarContourChartRenderer(*this).render(imagepath);
}



void PolarContourChart::insertLatexHeaderCode ( std::set<std::string>& hc ) const
{
    hc.insert("\\usepackage{graphicx}");
    hc.insert("\\usepackage{placeins}");
}


void PolarContourChart::writeLatexCode (
        std::ostream& f,
        const std::string& name,
        int level,
        const boost::filesystem::path& outputfilepath ) const
{
    path chart_file=cleanLatexImageFileName ( outputfilepath/ ( name+".png" ) ).string();

    generatePlotImage ( chart_file );

    //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
    f<<
     "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{"
        << make_relative ( outputfilepath, chart_file ).generic_path().string() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
     "\\FloatBarrier";
}


void PolarContourChart::exportDataToFile (
        const std::string& name,
        const boost::filesystem::path& outputdirectory ) const
{

    boost::filesystem::path fname ( outputdirectory/ ( name+".xyz" ) );
    arma::mat xyz = arma::zeros(x_.n_elem+1, y_.n_elem+1);
    for (int i=0; i<x_.n_elem; ++i) xyz(i+1,0)=x_(i);
    for (int i=0; i<y_.n_elem; ++i) xyz(0,i+1)=y_(i);
    for (int i=0; i<data_.n_rows; ++i)
        for (int j=0; j<data_.n_cols; ++j)
        {
            xyz(i+1, j+1)=data_(i,j);
        }
    xyz.save ( fname.string(), arma::raw_ascii );
}

rapidxml::xml_node<>* PolarContourChart::appendToNode
(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node
) const
{
    using namespace rapidxml;
    auto* child = ResultElement::appendToNode ( name, doc, node );

    appendAttribute(doc, *child, "rlabel", rlabel_);
    appendAttribute(doc, *child, "rmax", rmax_);
    appendAttribute(doc, *child, "cblabel", cblabel_);
    if (zClipMax_)
        appendAttribute(doc, *child, "zClipMax", *zClipMax_);
    if (zClipMin_)
        appendAttribute(doc, *child, "zClipMin", *zClipMin_);

    {
        auto& cln = appendNode(doc, *child, "contourLines" );
        for (const auto& cl: contourLines_)
        {
            auto& c = appendNode(doc, cln, "contourLine" );
            appendAttribute(doc, c, "value", cl);
        }
    }


    {
       auto& n = appendNode(doc, *child, "x" );
        writeMatToXMLNode ( x_, doc, n );
    }
    {
        auto& n = appendNode(doc, *child, "y" );
        writeMatToXMLNode ( y_, doc, n );
    }
    {
        auto& n = appendNode(doc, *child, "data" );
        writeMatToXMLNode ( data_, doc, n );
    }

    return child;
}


void PolarContourChart::readFromNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    )
{
    readBaseAttributesFromNode(name, doc, node);
    rlabel_=node.first_attribute("rlabel")->value();
    rmax_=toNumber<double>(node.first_attribute("rmax")->value());
    cblabel_=node.first_attribute("cblabel")->value();
    if (auto * n=node.first_attribute("zClipMax"))
    {
        zClipMax_=boost::lexical_cast<double>(n->value());
    }
    if (auto * n=node.first_attribute("zClipMin"))
    {
        zClipMin_=boost::lexical_cast<double>(n->value());
    }

    if (auto * n=node.first_node("contourLines"))
    {
        for (auto *e = node.first_node("contourLine");
             e; e = e->next_sibling("contourLine"))
        {
            contourLines_.push_back(
                        toNumber<double>(
                            e->first_attribute("value")->value())
                        );
        }
    }

    if (auto * n=node.first_node("x"))
    {
        std::istringstream iss(n->value());
        x_.load(iss, arma::raw_ascii);
    }
    if (auto * n=node.first_node("y"))
    {
        std::istringstream iss(n->value());
        y_.load(iss, arma::raw_ascii);
    }
    if (auto * n=node.first_node("data"))
    {
        std::istringstream iss(n->value());
        data_.load(iss, arma::raw_ascii);
    }
}

ResultElementPtr PolarContourChart::clone() const
{
    auto res = std::make_shared<PolarContourChart>(
                rlabel_, cblabel_,
                rmax_, *this,
                shortDescription().simpleLatex(),
                longDescription().simpleLatex(),
                contourLines_,
                zClipMin_, zClipMax_ );
    res->setOrder ( order() );
    return res;
}


void addPolarContourPlot
(
    std::shared_ptr<ResultElementCollection> results,
    const std::string& resultelementname,
    const std::string& rlabel,
    const std::string& cblabel,
    double rmax,
    const PlotField& plf,
    const std::string& shortDescription,
    const std::vector<double>& contourLines,
    boost::optional<double> zClipMin,
    boost::optional<double> zClipMax
)
{
    results->insert (
                resultelementname,
                new PolarContourChart
                (
                    rlabel, cblabel,
                    rmax,
                    plf,
                    shortDescription, "",
                    contourLines,
                    zClipMin, zClipMax
                    )
                );
}







} // namespace insight
