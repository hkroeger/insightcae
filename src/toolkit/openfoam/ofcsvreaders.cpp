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

#include "ofcsvreaders.h"
#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/tools.h"
#include "base/translations.h"

#include "base/units.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/createpatch.h"

#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <map>
#include <cmath>
#include <limits>

#include "vtkSTLReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkCellData.h"
#include "base/warningdispatcher.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{


arma::mat readTextFile(std::istream& f)
{
  CurrentExceptionContext ex("reading tabular data from input stream");

  arma::mat data;
  std::vector< std::vector<double> > fd;

  std::string line;
  int iline=0;
  while (getline(f, line))
  {
    iline++;
    CurrentExceptionContext ex(insight::VerbosityLevel::Loops, str(format("reading line %d (containing \"%s\")")%iline%line), false);

    algorithm::trim_left(line);
    char fc; istringstream(line) >> fc; // get first char
    if ( (line.size()==0) || (fc=='#') )
    {
      // comment
    }
    else
    {
      erase_all ( line, "(" );
      erase_all ( line, ")" );
      replace_all ( line, ",", " " );
      replace_all ( line, "\t", " " );
      while (line.find("  ")!=std::string::npos)
      {
        replace_all ( line, "  ", " " );
      }

      std::vector<std::string> strs;
      boost::split(strs, line, is_any_of(" "));

//      for (const auto& s: strs) std::cout<<s<<" >> "; std::cout<<std::endl;

      std::vector<double> vals;
      transform(strs.begin(), strs.end(), std::back_inserter(vals),
                [](const std::string& s) { return insight::toNumber<double>(s); });

      fd.push_back(vals);
    }
  }

  if (fd.size()==0)
  {
    data=arma::mat();
  }
  else
  {
    data.reshape(fd.size(), fd[0].size());
    size_t ir=0;
    for (const auto& r: fd)
    {
      if (r.size()!=data.n_cols)
        throw insight::Exception(str(format("Wrong number of cols (%d) in data row %d. Expected %d.")
                                     %r.size()%ir%data.n_cols));
      else
      {
        for (size_t j=0;j<r.size(); j++)
          data(ir,j)=r[j];
      }
      ir++;
    }
  }

  return data;
}

arma::mat readParaviewCSV(const boost::filesystem::path& file, std::map<std::string, int>* headers)
{
  cout << "Reading "<<file<<endl;
    
  std::ifstream f(file.c_str());
  
  std::vector<double> data;
  
  std::string headerline;
  getline(f, headerline);
  std::vector<std::string> colnames;
  boost::split(colnames, headerline, boost::is_any_of(","));
  for(size_t i=0; i<colnames.size(); i++)
  {
    (*headers)[colnames[i]]=i;
  }
  
  while(!f.eof())
  {
    std::string line;
    getline(f, line);
    if (f.fail()) break;
    
    std::vector<std::string> cols;
    boost::split(cols, line, boost::is_any_of(","));
    for(size_t i=0; i<cols.size(); i++)
    {
      data.push_back(toNumber<double>(cols[i]));
    }
  }
  
  return arma::mat(data.data(), colnames.size(), data.size()/colnames.size()).t();
}

typedef std::map<std::string, int> ColumnDescription;

bool equal_columns(const ColumnDescription& c1, const ColumnDescription& c2)
{
  bool ok = (c1.size()==c2.size());
  for (const ColumnDescription::value_type& c1e: c1)
  {
//     cout<<"col="<<c1e.first<<" ("<<c1e.second<<") >>> ";
    ColumnDescription::const_iterator i2=c2.find(c1e.first);
    if (i2 == c2.end()) { ok=false; /*cout<<"not found"<<endl;*/ }
    else { ok = ok && (c1e.second == i2->second); /*cout<<i2->second<<endl;*/ }
  }
  return ok;
}

std::vector<arma::mat> readParaviewCSVs(const boost::filesystem::path& filetemplate, ColumnDescription* headers)
{  
  typedef std::map<std::string, std::vector< arma::mat> > AllData;
  AllData alldata;
  
  boost::regex fname_pattern(filetemplate.filename().stem().string() + "[0-9]+" + filetemplate.filename().extension().string());
  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( filetemplate.parent_path() );
	itr != end_itr; ++itr )
  {
    if ( is_regular_file(itr->status()) )
    {
//       cout<<"file: "<<itr->path().filename().string()<<endl;
      if ( boost::regex_match( itr->path().filename().string(), fname_pattern ) )
      {
// 	cout<<"OK"<<endl;
	std::map<std::string, int> thisheaders;
	arma::mat r = readParaviewCSV(itr->path().c_str(), &thisheaders);
// 	cout<< (r.n_rows) <<" & "<<thisheaders.size()<<endl;
	if ( (thisheaders.size()>0) && (r.n_rows>0))
	{
	  if (alldata.size()==0)
	  {
	    // insert all cols
	    for (const ColumnDescription::value_type& cd: thisheaders)
	    {
	      alldata[cd.first].push_back(r.col(cd.second));
	    }
	  }
	  else
	  {
	    std::set<std::string> vc;
	    for (const AllData::value_type& adt: alldata) vc.insert(adt.first);
	    for (const ColumnDescription::value_type& cdt: thisheaders)
	    {
	      AllData::iterator j=alldata.find(cdt.first); // try to find each column of this CSV in alldata
	      if (j!=alldata.end()) // if present, append
	      {
// 		cout<<"append "<<cdt.first<<endl;
		j->second.push_back(r.col(cdt.second));
		vc.erase(vc.find(cdt.first));
	      }
	    }
	    // remove all cols, that are not present
	    for (const std::string& vci: vc)
	    {
// 	      cout<<"Remove "<<vci<<endl;
	      alldata.erase(alldata.find(vci));
	    }
	  }
// 	  if (result.size()==0)
// 	  {
// 	    header=thisheaders;
// 	  }
// 	  else
// 	  {
// 	    if (!equal_columns(header, thisheaders))
// 	    {
// 	      throw insight::Exception("incompatible file columns!");
// 	    }
// 	  }
// 	  result.push_back(r);
	}
      } /*else { cout<<"NO"<<endl; }*/
    }
  }

//   if (headers) *headers=header;
//   return result;

  int np=0;
  if (alldata.size()>0) np=alldata.begin()->second.size();

  std::vector<arma::mat> result(np);
  
  if (headers)
  {
    int j=0;
    for (const AllData::value_type& cv: alldata)
    {
      (*headers)[cv.first]=j++;
    }
  }
  
  for (const AllData::value_type& cv: alldata)
  {
    const std::vector< arma::mat>& profs=cv.second;
    int k=0;
    for (const arma::mat& col: profs)
    {
      arma::mat& cumprof=result[k++];
      if (cumprof.n_cols==0) 
	cumprof=col;
      else
	cumprof=join_rows(cumprof, col); // append column
    }
  }
  
  return result;
}


}
