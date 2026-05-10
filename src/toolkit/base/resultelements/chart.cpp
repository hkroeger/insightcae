#include "chart.h"
#include "base/hierarchicalelement.h"
#include "base/resultelements/chartrenderer.h"

#include "base/tools.h"
#include "base/rapidxml.h"
#include <memory>
#include <sstream>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
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

PlotCurve::PlotCurve(const arma::mat& x, const arma::mat& y, const std::string& plaintextlabel, const std::string &style)
: plotcmd_(style),
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

PlotCurve::PlotCurve ( const arma::mat& x, const arma::mat& y, const std::string& plaintextlabel, const PlotCurveStyle& style )
  : style_(style),
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
    xy_ = ArmaMatCmpts{
        { arma::as_scalar(arma::min(xrange)), y },
        { arma::as_scalar(arma::max(xrange)), y }
    };
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

std::pair<double, double>
PlotCurve::significantMinMax(double lastPartFraction) const
{
    arma::mat ysig = xy_.rows(xy_.n_rows/3, xy_.n_rows-1).col(1);
    double mi=ysig.min();
    double ma=ysig.max();

    if (mi<0.) mi*=1.1; else mi*=0.9;
    if (ma>0.) ma*=1.1; else ma*=0.9;

    std::set<double> lims{ mi, 0., ma };

    return { *lims.begin(), *(--lims.end()) };
}

bool PlotCurve::operator==(const PlotCurve o) const
{
    if (plotcmd_!=o.plotcmd_)
        return false;

    if (plaintextlabel_!=o.plaintextlabel_)
        return false;

    if (!(style_==o.style_))
        return false;

    return xy_==o.xy_;
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

bool PlotCurveList::operator==(const PlotCurveList o) const
{
    if (o.size()!=size())
        return false;
    for (size_t i=0; i<size(); ++i)
    {
        if (!((*this)[i]==o[i]))
            return false;
    }
    return true;
}







defineType(Chart);
addToFactoryTable(ResultElement, Chart);



Chart::Chart(const std::string& shortdesc, const std::string& longdesc, const std::string& unit)
: ResultElement(shortdesc, longdesc, unit)
{
    setDisplayFullPage(true);
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
  ChartData{xlabel, ylabel, plc, addinit}
{
    setDisplayFullPage(true);
}

const ChartData* Chart::chartData() const
{
    return this;
}

void Chart::addCurve(const PlotCurve &pc)
{
    plc_.push_back(pc);
}

const PlotCurve &Chart::plotCurve(const std::string &plainTextLabel) const
{
    auto i=std::find_if(
                plc_.begin(), plc_.end(),
                [&plainTextLabel](const PlotCurve& pc)
                 { return pc.plaintextlabel_ == plainTextLabel; }
    );

    if (i==plc_.end())
    {
        std::string curveNames;
        for (const auto& c: plc_)
            curveNames+=" "+c.plaintextlabel_;
        throw insight::Exception(
                    "Curve "+plainTextLabel+" not found in chart data! "
                    "Available curves: "+curveNames
                    );
    }

    return *i;
}





void Chart::generatePlotImage( const boost::filesystem::path& imagepath ) const
{
  ChartRenderer::create(chartData())->render(imagepath);
}


void Chart::insertLatexHeaderCode(std::set<std::string>& h) const
{
  h.insert("\\usepackage{graphicx}");
  h.insert("\\usepackage{placeins}");
}


std::string Chart::latexRepresentation (
    const std::string& name,
    int,
    const FileStorageInfo& fsi ) const
{
    auto &addf = *fsi.additionalFiles;

    auto filename=cleanLatexImageFileName(name+".png");
    auto chart_file =
        (addf.directory/filename).string();

    generatePlotImage ( chart_file );

    std::ostringstream f;
    f<<
     "\n\nSee figure below.\n"
     "\\begin{figure}[!h]"
     "\\PlotFrame{keepaspectratio,width=\\textwidth}{"
        << (addf.directoryRelativePath/filename ).generic_string() << "}\n"
     "\\caption{"+shortDescription_.toLaTeX()+"}\n"
     "\\end{figure}"
     "\\FloatBarrier";

    return f.str();
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
        pc.xy_.save ( fname.string(), arma::raw_ascii );
        curveID++;
    }
}

int Chart::nChildren() const
{
    return 0;
}

bool Chart::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const Chart*>(&op))
    {
        return ChartData::operator==(*oa);
    }
    else
        return false;
}


rapidxml::xml_node<>* Chart::appendToNode
(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    const OutputProperties& outProps
) const
{
    using namespace rapidxml;
    xml_node<>* child =
        ResultElement::appendToNode ( name, doc, node, outProps );

    appendAttribute(doc, *child, "xlabel", xlabel_ );
    appendAttribute(doc, *child, "ylabel", ylabel_ );
    appendAttribute(doc, *child, "addinit", addinit_ );

    for ( const PlotCurve& pc: plc_ )
    {
        auto pcnode = appendNode(doc, *child, "PlotCurve");
        appendAttribute(doc, pcnode, "plaintextlabel", pc.plaintextlabel() );
        appendAttribute(doc, pcnode, "plotcmd", pc.plotcmd_ );
        writeMatToXMLNode ( pc.xy_, doc, pcnode );
    }

    return child;
}



const rapidxml::xml_node<>*
Chart::readFromNode(
    const string &name,
    const rapidxml::xml_node<> &parentNode )
{
  auto *child =
        ResultElement::readFromNode(name, parentNode);

  xlabel_=getMandatoryAttribute(*child, "xlabel");
  ylabel_=getMandatoryAttribute(*child, "ylabel");
  addinit_=getMandatoryAttribute(*child, "addinit");

  for (xml_node<> *e = child->first_node(); e; e = e->next_sibling())
  {
    auto plaintextlabel=getMandatoryAttribute(*e, "plaintextlabel");
    auto plotcmd=getMandatoryAttribute(*e, "plotcmd");

    std::string value_str=e->value();
    std::istringstream iss(value_str);
    arma::mat xy;
    xy.load(iss, arma::raw_ascii);

    plc_.push_back(PlotCurve(xy, plaintextlabel, plotcmd));
  }
  return child;
}




std::unique_ptr<hierarchicalData::Element> Chart::cloneUninitialized() const
{
    auto res=std::make_unique<Chart>(
        xlabel_, ylabel_, plc_,
        shortDescription().simpleLatex(),
        longDescription().simpleLatex(),
        addinit_ );
    res->setOrder ( order() );
    return res;
}




std::string yRangeExpression(double mi, double ma, double boundaryBySpan)
{
    if (mi>ma) std::swap(mi, ma);
    double span = ma-mi;
    double bnd =
        std::max(1., boundaryBySpan*span);
    return str ( boost::format ( "set yrange [%g:%g]" )
               % ( mi-bnd ) % ( ma+bnd ) );
}




insight::ResultElement& addPlot
(
    ResultElementCollection& results,
    const boost::filesystem::path& workdir,
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

    return results.insert<Chart> (
        resultelementname,
        xlabel, ylabel, plc,
        shortDescription, "",
        precmd
        );
}

bool ChartData::operator==(const ChartData o) const
{
    if (xlabel_!=o.xlabel_) return false;
    if (ylabel_!=o.ylabel_) return false;
    if (addinit_!=o.addinit_) return false;
    return plc_==o.plc_;
}

bool PlotCurveStyle::operator==(const PlotCurveStyle &o) const
{
    if (color_!=o.color_) return false;
    if (lineWidth_!=o.lineWidth_) return false;
    if (dashType_!=o.dashType_) return false;
    if (withPoints_!=o.withPoints_) return false;
    if (withLines_!=o.withLines_) return false;
    if (errorLines_!=o.errorLines_) return false;
    if (title_!=o.title_) return false;
    if (ax_y_!=o.ax_y_) return false;
    return true;
}




} // namespace insight
