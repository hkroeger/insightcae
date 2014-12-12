/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#include "resultset.h"
#include "base/latextools.h"

#include <fstream>

//#include "boost/gil/gil_all.hpp"
//#include "boost/gil/extension/io/jpeg_io.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/foreach.hpp"
#include "boost/assign.hpp"
#include "boost/date_time.hpp"
#include <boost/graph/buffer_concepts.hpp>
#include "boost/filesystem.hpp"
#include "boost/regex.hpp"

#include "boost/format.hpp"

#include "gnuplot-iostream.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;

namespace insight
{
  
  
string latex_subsection(int level)
{
  string cmd="\\";
  if (level==2) cmd+="paragraph";
  else if (level==3) cmd+="subparagraph";
  else if (level>3) cmd="";
  else
  {
    for (int i=0; i<min(2,level); i++)
      cmd+="sub";
    cmd+="section";
  }
  return cmd;
}

defineType(ResultElement);
defineFactoryTable(ResultElement, ResultElement::ResultElementConstrP);

ResultElement::ResultElement(const ResultElement::ResultElementConstrP& par)
: shortDescription_(boost::get<0>(par)),
  longDescription_(boost::get<1>(par)),
  unit_(boost::get<2>(par))
{}

ResultElement::~ResultElement()
{}


void ResultElement::writeLatexHeaderCode(std::ostream& f) const
{
}

void ResultElement::writeLatexCode(ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{
}

void ResultElement::exportDataToFile(const std::string& name, const boost::filesystem::path& outputdirectory) const
{
}

defineType(Image);
addToFactoryTable(ResultElement, Image, ResultElement::ResultElementConstrP);


Image::Image(const ResultElementConstrP& par)
: ResultElement(par)
{
}

Image::Image(const boost::filesystem::path& location, const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc)
: ResultElement(ResultElementConstrP(shortDesc, longDesc, "")),
  imagePath_(absolute(value, location))
{
}

void Image::writeLatexHeaderCode(std::ostream& f) const
{
  f<<"\\usepackage{graphicx}\n";
  f<<"\\usepackage{placeins}\n";
}

void Image::writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{
  //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
  f<< 
  "\n\nSee figure below.\n"
  "\\begin{figure}[!h]"
  "\\PlotFrame{keepaspectratio,width=\\textwidth}{" << make_relative(outputfilepath, imagePath_).c_str() << "}\n"
  "\\caption{"+cleanSymbols(shortDescription_)+"}\n"
  "\\end{figure}"
  "\\FloatBarrier";
}

ResultElement* Image::clone() const
{
  return new Image(imagePath_.parent_path(), imagePath_, shortDescription_, longDescription_);
}
  
defineType(Comment);
addToFactoryTable(ResultElement, Comment, ResultElement::ResultElementConstrP);

Comment::Comment(const ResultElementConstrP& par)
: ResultElement(par)
{}

Comment::Comment(const std::string& value, const std::string& shortDesc, const std::string& longDesc)
: ResultElement(ResultElementConstrP(shortDesc, longDesc, "")),
  value_(value)
{
}

void Comment::writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{
  f << value_ <<endl;
}

ResultElement* Comment::clone() const
{
  return new Comment(value_, shortDescription_, longDescription_);
}
  
defineType(ScalarResult);
addToFactoryTable(ResultElement, ScalarResult, ResultElement::ResultElementConstrP);


ScalarResult::ScalarResult(const ResultElementConstrP& par)
: NumericalResult< double >(par)
{
}

ScalarResult::ScalarResult(const double& value, const string& shortDesc, const string& longDesc, const string& unit)
: NumericalResult< double >(value, shortDesc, longDesc, unit)
{}

void ScalarResult::writeLatexCode(ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{
  f.setf(ios::fixed,ios::floatfield);
  f.precision(3);
  f << value_ << unit_;
}

ResultElement* ScalarResult::clone() const
{
  return new ScalarResult(value_, shortDescription_, longDescription_, unit_);
}


defineType(TabularResult);
addToFactoryTable(ResultElement, TabularResult, ResultElement::ResultElementConstrP);


TabularResult::TabularResult(const ResultElementConstrP& par)
: ResultElement(par)
{
}

TabularResult::TabularResult
(
  const std::vector<std::string>& headings, 
  const Table& rows,
  const std::string& shortDesc, 
  const std::string& longDesc,
  const std::string& unit
)
: ResultElement(ResultElementConstrP(shortDesc, longDesc, unit))
{
  setTableData(headings, rows);
}

TabularResult::TabularResult
(
  const std::vector<std::string>& headings, 
  const arma::mat& rows,
  const std::string& shortDesc, 
  const std::string& longDesc,
  const std::string& unit
)
: ResultElement(ResultElementConstrP(shortDesc, longDesc, unit))
{
  Table t;
  for (int i=0; i<rows.n_rows; i++)
  {
    Row r;
    for (int j=0; j<rows.n_cols; j++)
    {
      r.push_back(rows(i,j));
    }
    t.push_back(r);
  }
  setTableData(headings, t);
}

void TabularResult::setCellByName(TabularResult::Row& r, const string& colname, double& value)
{
  std::vector<std::string>::const_iterator ii=std::find(headings_.begin(), headings_.end(), colname);
  if (ii==headings_.end())
  {
    std::ostringstream msg;
    msg<<"Tried to write into column "+colname+" but this does not exist! Existing columns are:"<<endl;
    BOOST_FOREACH(const std::string& n, headings_)
    {
      msg<<n<<endl;
    }
    insight::Exception(msg.str());
  }
  int i= ii - headings_.begin();
  r[i]=value;
}


arma::mat TabularResult::toMat() const
{
  arma::mat res;
  res.resize(rows_.size(), rows_[0].size());
  int i=0;
  BOOST_FOREACH(const std::vector<double>& row, rows_)
  {
    int j=0;
    BOOST_FOREACH(double v, row)
    {
      cout<<"res("<<i<<","<<j<<")="<<v<<endl;
      res(i, j++)=v;
    }
    i++;
  }
  return res;
}

void TabularResult::writeGnuplotData(std::ostream& f) const
{
  f<<"#";
  BOOST_FOREACH(const std::string& head, headings_)
  {
     f<<" \""<<head<<"\"";
  }
  f<<std::endl;

  BOOST_FOREACH(const std::vector<double>& row, rows_)
  {
    BOOST_FOREACH(double v, row)
    {
      f<<" "<<v;
    }
    f<<std::endl;
  }

}

ResultElement* TabularResult::clone() const
{
  return new TabularResult(headings_, rows_, shortDescription_, longDescription_, unit_);
}

void TabularResult::writeLatexHeaderCode(ostream& f) const
{
  insight::ResultElement::writeLatexHeaderCode(f);
  f<<"\\usepackage{longtable}\n";
  f<<"\\usepackage{placeins}\n";
}

void TabularResult::writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{

  f<<
  "\\begin{longtable}{";
  BOOST_FOREACH(const std::string& h, headings_)
  {
    f<<"c";
  }
  f<<"}\n";
  
  for (std::vector<std::string>::const_iterator i=headings_.begin(); i!=headings_.end(); i++)
  {
    if (i!=headings_.begin()) f<<" & ";
    f<<*i;
  }
  f<<
  "\\\\\n"
  "\\hline\n"
  "\\endfirsthead\n"
  "\\endhead\n";
  for (TabularResult::Table::const_iterator i=rows_.begin(); i!=rows_.end(); i++)
  {
    if (i!=rows_.begin()) f<<"\\\\\n";
    for (std::vector<double>::const_iterator j=i->begin(); j!=i->end(); j++)
    {
      if (j!=i->begin()) f<<" & ";
      f<<*j;
    }
  }
  f<<
  "\\end{longtable}\n"
  "\\newpage\n";  // page break algorithm fails after too short "longtable"
}

defineType(AttributeTableResult);
addToFactoryTable(ResultElement, AttributeTableResult, ResultElement::ResultElementConstrP);

AttributeTableResult::AttributeTableResult(const ResultElementConstrP& par)
: ResultElement(par)
{
}
  
AttributeTableResult::AttributeTableResult
(
  AttributeNames names,
  AttributeValues values, 
  const std::string& shortDesc, 
  const std::string& longDesc,
  const std::string& unit
)
: ResultElement(ResultElementConstrP(shortDesc, longDesc, unit))
{
  setTableData(names, values);
}
  
  
void AttributeTableResult::writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{
  f<<
  "\\begin{tabular}{lc}\n"
  "Attribute & Value \\\\\n"
  "\\hline\\\\";
  for(int i=0; i<names_.size(); i++)
  {
    f<<names_[i]<<" & "<<values_[i]<<"\\\\"<<endl;
  }
  f<<"\\end{tabular}\n";
}
  
ResultElement* AttributeTableResult::clone() const
{
  return new AttributeTableResult(names_, values_, 
					       shortDescription_, longDescription_, unit_);
}

ResultElementPtr polynomialFitResult
(
  const arma::mat& coeffs, 
  const std::string& xvarName, 
  const std::string& shortDesc, 
  const std::string& longDesc,
  int minorder  
)
{
  std::vector<std::string> header;
  TabularResult::Row cr;
  for (int i=coeffs.n_rows-1; i>=0; i--)
  {
    int order=minorder+i;
    if (order==0)
      header.push_back("$1$");
    else if (order==1)
      header.push_back("$"+xvarName+"$");
    else
      header.push_back("$"+xvarName+"^{"+lexical_cast<string>(order)+"}$");
    cr.push_back(coeffs(i));
  }
  
  return ResultElementPtr
  (
    new TabularResult
    (
      header, 
      list_of<TabularResult::Row>(cr).convert_to_container<TabularResult::Table>(),
      shortDesc, longDesc, ""
    )
  );
}
  
ResultSet::ResultSet
(
  const ParameterSet& p,
  const std::string& title,
  const std::string& subtitle,
  const std::string* date,
  const std::string* author
)
: ResultElement(ResultElementConstrP("", "", "")),
  p_(p),
  title_(title),
  subtitle_(subtitle)
{
  if (date) 
    date_=*date;
  else
  {
    using namespace boost::gregorian;
    date_=to_iso_extended_string(day_clock::local_day());
  }
  
  if (author) 
    author_=*author;
  else
  {
    author_=getenv("USER");
  }
}

ResultSet::~ResultSet()
{}

ResultSet::ResultSet(const ResultSet& other)
: ptr_map< std::string, ResultElement>(other),
  ResultElement(ResultElementConstrP("", "", "")),
  p_(other.p_),
  title_(other.title_),
  subtitle_(other.subtitle_),
  author_(other.author_),
  date_(other.date_)
{
}


void ResultSet::transfer(const ResultSet& other)
{
  ptr_map< std::string, ResultElement>::operator=(other);
  p_=other.p_;
  title_=other.title_;
  subtitle_=other.subtitle_;
  author_=other.author_;
  date_=other.date_;
}

void ResultSet::writeLatexHeaderCode(std::ostream& f) const
{
  for (ResultSet::const_iterator i=begin(); i!=end(); i++)
  {
    i->second->writeLatexHeaderCode(f);
  }  
}

void ResultSet::writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{
  if (level>0)
  {
    f << title_ << "\n\n";

    f << subtitle_ << "\n\n";
  }
  
  f << latex_subsection(level) << "{Input Parameters}\n";
  
  f<<p_.latexRepresentation();
  
  f << latex_subsection(level) << "{Numerical Result Summary}\n";
  for (ResultSet::const_iterator i=begin(); i!=end(); i++)
  {
    f << latex_subsection(level+1) << "{" << cleanSymbols(i->first) << "}\n";
    f << cleanSymbols(i->second->shortDescription()) << "\n\n";
    
    std::string subelemname=i->first;
    if (name!="")
      subelemname=name+"__"+i->first;
    
    i->second->writeLatexCode(f, subelemname, level+2, outputfilepath);
    
    f << "\n\n" << cleanSymbols(i->second->longDescription()) << "\n\n";
    f << endl;
  }
}

void ResultSet::exportDataToFile(const std::string& name, const boost::filesystem::path& outputdirectory) const
{
  path outsubdir(outputdirectory/name);
  create_directory(outsubdir);
  for (ResultSet::const_iterator i=begin(); i!=end(); i++)
  {
    i->second->exportDataToFile(i->first, outsubdir);
  }
}


void ResultSet::writeLatexFile(const boost::filesystem::path& file) const
{
  path filepath(absolute(file));
  
  {
    std::ofstream f(filepath.c_str());
    f<<"\\documentclass[a4paper,10pt]{scrartcl}\n";
    f<<"\\newcommand{\\PlotFrameB}[2]{%\n"
    <<"\\includegraphics[#1]{#2}\\endgroup}\n"
    <<"\\def\\PlotFrame{\\begingroup\n"
    <<"\\catcode`\\_=12\n"
    <<"\\PlotFrameB}\n"
    <<"\\usepackage{hyperref}\n"
    <<"\\usepackage{fancyhdr}\n"
    <<"\\pagestyle{fancy}\n";
    
    writeLatexHeaderCode(f);
    
    f<<
    "\\begin{document}\n"
    //"\\title{"<<title_<<"}\n"
    //"\\subtitle{"<<subtitle_<<"}\n"
    "\\title{"<<title_<<"\\\\\n"
    "\\vspace{0.5cm}\\normalsize{"<<subtitle_<<"}}\n"
    "\\date{"<<date_<<"}\n"
    "\\author{"<<author_<<"}\n"
    "\\maketitle\n"
    "\\tableofcontents\n";
      
    writeLatexCode(f, "", 0, filepath.parent_path());
    
    f<<
    "\\end{document}\n";
  }
  
  {
    path outdir(filepath.parent_path()/("report_data_"+filepath.stem().string()));
    create_directory(outdir);
    for (ResultSet::const_iterator i=begin(); i!=end(); i++)
    {
      i->second->exportDataToFile(i->first, outdir);
    }
  }
}

ResultElement* ResultSet::clone() const
{
  std::auto_ptr<ResultSet> nr(new ResultSet(p_, title_, subtitle_, &author_, &date_));
  for (ResultSet::const_iterator i=begin(); i!=end(); i++)
  {
    cout<<i->first<<endl;
    std::string key(i->first);
    nr->insert(key, i->second->clone());
  }
  return nr.release();
}

PlotCurve::PlotCurve()
{
}

PlotCurve::PlotCurve(const char* plotcmd)
: plotcmd_(plotcmd)
{
}

PlotCurve::PlotCurve(const std::vector<double>& x, const std::vector<double>& y, const std::string& plotcmd)
: xy_
  (
    join_rows( arma::mat(x.data(), x.size(), 1), arma::mat(y.data(), y.size(), 1) )
  ), 
  plotcmd_(plotcmd)
{
}

PlotCurve::PlotCurve(const arma::mat& xy, const std::string& plotcmd)
: xy_(xy), plotcmd_(plotcmd)
{}


std::string PlotCurve::title() const
{
  boost::regex re(".*t *'(.*)'.*");
  boost::smatch str_matches;
  std::cout<<plotcmd_<<std::endl;
  if (boost::regex_match(plotcmd_, str_matches, re))
  {
    std::cout<<" <> "<<str_matches[1]<<std::endl;
    return str_matches[1];
  }
  else return "";
}
  
void addPlot
(
  ResultSetPtr& results, 
  const boost::filesystem::path& workdir,
  const std::string& resultelementname,
  const std::string& xlabel,
  const std::string& ylabel,
  const PlotCurveList& plc,
  const std::string& shortDescription,
  const std::string& addinit
)
{
  
  results->insert(resultelementname,
    std::auto_ptr<Chart>(new Chart
    (
      xlabel, ylabel, plc,
      shortDescription, "",
      addinit
  )));
  
//   std::string chart_file_name=(workdir/(resultelementname+".png")).string();
//   //std::string chart_file_name_i=(workdir/(resultelementname+".ps")).string();
//   
//   {
//     Gnuplot gp;
//     
//     //gp<<"set terminal postscript color;";
//     //gp<<"set output '"<<chart_file_name_i<<"';";
//     gp<<"set terminal pngcairo; set termoption dash;";
//     gp<<"set output '"<<chart_file_name<<"';";
// 
//     gp<<"set linetype  1 lc rgb '#0000FF' lw 1;"
// 	"set linetype  2 lc rgb '#8A2BE2' lw 1;"
// 	"set linetype  3 lc rgb '#A52A2A' lw 1;"
// 	"set linetype  4 lc rgb '#E9967A' lw 1;"
// 	"set linetype  5 lc rgb '#5F9EA0' lw 1;"
// 	"set linetype  6 lc rgb '#006400' lw 1;"
// 	"set linetype  7 lc rgb '#8B008B' lw 1;"
// 	"set linetype  8 lc rgb '#696969' lw 1;"
// 	"set linetype  9 lc rgb '#DAA520' lw 1;"
// 	"set linetype cycle  9;";
// 
//     gp<<addinit<<";";
//     gp<<"set xlabel '"<<xlabel<<"'; set ylabel '"<<ylabel<<"'; set grid; ";
//     gp<<"plot 0 not lt -1";
//     BOOST_FOREACH(const PlotCurve& pc, plc)
//     {
//       if (pc.xy_.n_rows>0)
// 	gp<<", '-' "<<pc.plotcmd_;
//       else
// 	gp<<", "<<pc.plotcmd_;
//     }
//     gp<<endl;
//     BOOST_FOREACH(const PlotCurve& pc, plc)
//     {
//       if (pc.xy_.n_rows>0)
// 	gp.send1d(pc.xy_);
//     }
//   }
// 
//   results->insert(resultelementname,
//     std::auto_ptr<Image>(new Image
//     (
//     workdir, chart_file_name, 
//     shortDescription, ""
//   )));
}



defineType(Chart);
addToFactoryTable(ResultElement, Chart, ResultElement::ResultElementConstrP);

Chart::Chart(const ResultElement::ResultElementConstrP& par)
: ResultElement(par)
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
: ResultElement(ResultElementConstrP(shortDesc, longDesc, "")),
  xlabel_(xlabel),
  ylabel_(ylabel),
  plc_(plc),
  addinit_(addinit)
{
}

void Chart::generatePlotImage(const path& imagepath) const
{
//   std::string chart_file_name=(workdir/(resultelementname+".png")).string();
    
  {
    Gnuplot gp;
    
    //gp<<"set terminal postscript color;";
    //gp<<"set output '"<<chart_file_name_i<<"';";
    gp<<"set terminal pngcairo; set termoption dash;";
    gp<<"set output '"<<absolute(imagepath).string()<<"';";

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

    gp<<addinit_<<";";
    gp<<"set xlabel '"<<xlabel_<<"'; set ylabel '"<<ylabel_<<"'; set grid; ";
    gp<<"plot 0 not lt -1";
    BOOST_FOREACH(const PlotCurve& pc, plc_)
    {
      if (pc.xy_.n_rows>0)
	gp<<", '-' "<<pc.plotcmd_;
      else
	gp<<", "<<pc.plotcmd_;
    }
    gp<<endl;
    BOOST_FOREACH(const PlotCurve& pc, plc_)
    {
      if (pc.xy_.n_rows>0)
	gp.send1d(pc.xy_);
    }
  }
}

  
void Chart::writeLatexHeaderCode(std::ostream& f) const
{
  f<<"\\usepackage{graphicx}\n";
  f<<"\\usepackage{placeins}\n";
}

void Chart::writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const
{
  path chart_file=cleanLatexImageFileName(outputfilepath/(name+".png")).string();
  
  generatePlotImage(chart_file);
  
  //f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << cleanSymbols(imagePath_.c_str()) << "}\n";
  f<< 
  "\n\nSee figure below.\n"
  "\\begin{figure}[!h]"
  "\\PlotFrame{keepaspectratio,width=\\textwidth}{" << make_relative(outputfilepath, chart_file).c_str() << "}\n"
  "\\caption{"+cleanSymbols(shortDescription_)+"}\n"
  "\\end{figure}"
  "\\FloatBarrier";
}

void Chart::exportDataToFile(const std::string& name, const boost::filesystem::path& outputdirectory) const
{
  int curveID=0;
  BOOST_FOREACH(const PlotCurve& pc, plc_)
  {
    std::string suf=pc.title();
    replace_all(suf, "/", "_");
    if (suf=="")
      suf=str(format("curve%d")%curveID);
    
    boost::filesystem::path fname(outputdirectory/(name+"__"+suf+".xy"));
    
    std::ofstream f(fname.c_str());
    pc.xy_.save(fname.string(), arma::raw_ascii);
    curveID++;
  }
}

  
ResultElement* Chart::clone() const
{
  return new Chart(xlabel_, ylabel_, plc_, shortDescription(), longDescription(), addinit_);
}



PlotField::PlotField()
{
}

PlotField::PlotField(const arma::mat& xy, const std::string& plotcmd)
: xy_(xy), plotcmd_(plotcmd)
{}


void addContourPlot
(
  ResultSetPtr& results, 
  const boost::filesystem::path& workdir,
  const std::string& resultelementname,
  const std::string& xlabel,
  const std::string& ylabel,
  const PlotFieldList& plc,
  const std::string& shortDescription,
  const std::string& addinit
)
{
  std::string chart_file_name=(workdir/(resultelementname+".png")).string();
  //std::string chart_file_name_i=(workdir/(resultelementname+".ps")).string();
  
  {
    Gnuplot gp;
    
    //gp<<"set terminal postscript color;";
    //gp<<"set output '"<<chart_file_name_i<<"';";
    gp<<"set terminal pngcairo; set termoption dash;";
    gp<<"set output '"<<chart_file_name<<"';";

    gp<<addinit<<";";
    gp<<"set xlabel '"<<xlabel<<"'; set ylabel '"<<ylabel<<"'; set grid; ";
    gp<<"splot ";
    BOOST_FOREACH(const PlotCurve& pc, plc)
    {
      gp<<"'-' "<<pc.plotcmd_;
    }
    gp<<endl;
    BOOST_FOREACH(const PlotCurve& pc, plc)
    {
      gp.send(pc.xy_);
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
  results->insert(resultelementname,
    std::auto_ptr<Image>(new Image
    (
    workdir, chart_file_name, 
    shortDescription, ""
  )));
}

}



