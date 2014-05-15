/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#include "analysiscaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

#include "gnuplot-iostream.h"

namespace insight
{

template<class TPC>
TPCArray<TPC>::TPCArray(OpenFOAMCase& cm, Parameters const &p )
: OpenFOAMCaseElement(cm, p.name_prefix()+"TPCArray"),
  p_(p)
{
  using namespace std;
  using namespace boost;
  using namespace boost::filesystem;
  using namespace boost::assign;
  using namespace boost::fusion;

  int n_r=10;
  for (int i=1; i<n_r; i++) // omit first and last
  {
    double x = double(i)/(n_r);
    double r = -::cos(M_PI*(0.5+0.5*x))*p_.R();
    
    cout<<"Creating tpc FOs for set "<<p_.name_prefix()<<" at r="<<r<<endl;
    r_.push_back(r);
    
    tpc_tan_.push_back(new TPC(cm, getTanParameters(i)));
    tpc_ax_.push_back(new TPC(cm, getAxParameters(i)));    
  }  
}

template<class TPC>
void TPCArray<TPC>::addIntoDictionaries(OFdicts& dictionaries) const
{
  using namespace std;
  using namespace boost;
  using namespace boost::filesystem;
  using namespace boost::assign;
  using namespace boost::fusion;

  BOOST_FOREACH(const TPC& tpc, tpc_tan_)
  {
    tpc.addIntoDictionaries(dictionaries);
  }
  BOOST_FOREACH(const TPC& tpc, tpc_ax_)
  {
    tpc.addIntoDictionaries(dictionaries);
  }
}



template<class TPC>
void TPCArray<TPC>::evaluate(
  OpenFOAMCase& cm, 
  const boost::filesystem::path& location, 
  ResultSetPtr& results,
  const std::string& shortDescription
) const
{
  evaluateSingle(cm, location, results, 
		  p_.name_prefix()+"_tan", 
		  p_.tanSpan(), axisTitleTan(),
		  tpc_tan_, 
		  shortDescription+" (along tangential direction)"
		);
  
  evaluateSingle(cm, location, results, 
		  p_.name_prefix()+"_ax", 
		  p_.axSpan(),  axisTitleAx(),
		  tpc_ax_, 
		  shortDescription+" (along axial direction)" //"two-point correlation of velocity along axial direction at different radii"
		);
}

template<class TPC>
void TPCArray<TPC>::evaluateSingle
(
  OpenFOAMCase& cm, const boost::filesystem::path& location, 
  ResultSetPtr& results, 
  const std::string& name_prefix,
  double span,
  const std::string& axisLabel,
  const boost::ptr_vector<TPC>& tpcarray,
  const std::string& shortDescription
) const
{
  using namespace std;
  using namespace boost;
  using namespace boost::filesystem;
  using namespace boost::assign;
  using namespace boost::fusion;

  int nk=9;
  int nr=tpcarray.size();
  
  arma::mat L(r_.data(), r_.size(), 1);
  L=arma::join_rows(L, arma::zeros(r_.size(), nk)); // append nk column with 0's
  
  // Produce plot for each component of the tensor:
  // For reach component: setup plot data first
  PlotCurveList tpc_curves[nk] /*, tpc_regression_curves[nk],*/;
  int ir=0;
  BOOST_FOREACH(const TPC& tpc, tpcarray)
  {
    boost::ptr_vector<arma::mat> res=twoPointCorrelation::readCorrelations(cm, location, tpc.name());
    if (res[0].n_rows<1)
    {
      results->insert(name_prefix,
	std::auto_ptr<Comment>(new Comment
	(
	"(No data was available for evaluation)", 
	shortDescription, ""
      )));
      return;
    }
    
    // append profile of this radius to chart of this component
    for (int k=0; k<nk; k++)
    {
      //cmd[k]<<", '-' w p lt "<<ir+1<<" t 'r="<<r_[ir]<<"', '-' w l lt "<<ir+1<<" t 'r="<<r_[ir]<<" (fit)'";
      arma::mat data
      (
	join_rows
	(
	  arma::linspace<arma::mat>(0, span, p_.np()),
	  res[k+1].row(res[k+1].n_rows-1).t() // one TPC profile per row (one row per output time step), get the last row
	)
      );
      data.col(1) /= data(0,1); // Normalize two-point correlation values (y)
      
      CorrelationFunctionModel m;
      cout<<"Fitting tpc profile for set "<<p_.name_prefix()<<" at radius "<<ir<<" (r="<<r_[ir]<<"), component k="<<k<<" ("<<cmptNames[k]<<")"<<endl;
      nonlinearRegression(data.col(1), data.col(0), m);
      arma::mat regressiondata
      (
	join_rows
	(
	  data.col(0),
	  m.evaluateObjective(data.col(0))
	)
      );
      
      L(ir, 1+k)=m.lengthScale();
      
      tpc_curves[k].push_back
      (
	PlotCurve(data, "w p lt "+lexical_cast<std::string>(ir+1)+" t 'r="+lexical_cast<std::string>(r_[ir])+"'")
      );
      
      tpc_curves[k].push_back
      (
	PlotCurve(regressiondata, "w l lt "+lexical_cast<std::string>(ir+1)+" t 'r="+lexical_cast<std::string>(r_[ir])+" (fit)'")
      );
      
    }
    ir++;
  }
  
  // Produces plot of two-point correlation functions
  for (int k=0; k<nk; k++)
  {
    addPlot
    (
      results, location, name_prefix+"_"+cmptNames[k],
      axisLabel, "<R_"+std::string(cmptNames[k])+">",
      tpc_curves[k],
      shortDescription+", two-point correlation function for component "+cmptNames[k]
    );
  }
  

  //produce plots of diag L profiles
  {
    PlotCurveList L_diag_curves;
    std::vector<double> ks=list_of<double>(0)(4)(8);    
    BOOST_FOREACH(int k, ks)
    {
      L_diag_curves.push_back
      (
	PlotCurve
	(
	  arma::mat(join_rows(L.col(0), L.col(k+1))),
	  "w lp t 'L_"+std::string(cmptNames[k])+"'"
	)
      );
    }
    
    addPlot
    (
      results, location, name_prefix+"_L_diag",
      "Radius [length]", "L [length]",
      L_diag_curves,
      shortDescription+", autocorrelation lengths"
    );
  }

  //produce plots of off-diag L profiles
  {
    PlotCurveList L_offdiag_curves;
    std::vector<double> ks=list_of<double>(1)(2)(3)(5)(6)(7);
    
    BOOST_FOREACH(int k, ks)
    {
      L_offdiag_curves.push_back
      (
	PlotCurve
	(
	  arma::mat(join_rows(L.col(0), L.col(k+1))),
	  "w lp t 'L_"+std::string(cmptNames[k])+"'"
	)
      );
    }
    
    addPlot
    (
      results, location, name_prefix+"_L_offdiag",
      "Radius [length]", "L [length]",
      L_offdiag_curves,
      shortDescription+", cross-correlation lengths"
    );
  }
  
   /* 
  // create one plot per component, with the profiles for all radii overlayed
  {
    Gnuplot gp[nk];
    std::ostringstream cmd[nk];
    arma::mat data[nk], regressions[nk];
    
    for(int k=0; k<nk; k++)
    {
      std::string chart_name=name_prefix+"_"+cmptNames[k];
      std::string chart_file_name=chart_name+".png";
      
      gp[k]<<"set terminal png; set output '"<<chart_file_name<<"';";
      gp[k]<<"set xlabel '"<<axisLabel<<"'; set ylabel '<R_"<<cmptNames[k]<<">'; set grid; ";
      cmd[k]<<"plot 0 not lt -1";
      data[k]=arma::zeros(p_.np(), nr+1);
      data[k].col(0)=arma::linspace<arma::mat>(0, span, p_.np());
      regressions[k]=arma::zeros(p_.np(), nr+1);
      regressions[k].col(0)=arma::linspace<arma::mat>(0, span, p_.np());

      results->insert(chart_name,
	std::auto_ptr<Image>(new Image
	(
	chart_file_name, 
	shortDescription, ""
      )));

    }
    int ir=0;
    BOOST_FOREACH(const TPC& tpc, tpcarray)
    {
      boost::ptr_vector<arma::mat> res=twoPointCorrelation::readCorrelations(cm, location, tpc.name());
      
      // append profile of this radius to chart of this component
      for (int k=0; k<nk; k++)
      {
	cmd[k]<<", '-' w p lt "<<ir+1<<" t 'r="<<r_[ir]<<"', '-' w l lt "<<ir+1<<" t 'r="<<r_[ir]<<" (fit)'";
	data[k].col(ir+1) = res[k+1].row(res[k+1].n_rows-1).t();
	data[k].col(ir+1) /= data[k].col(ir+1)(0); // Normalize
	
	CorrelationFunctionModel m;
	cout<<"Fitting tpc profile for set "<<p_.name_prefix()<<" at radius "<<ir<<" (r="<<r_[ir]<<"), component k="<<k<<" ("<<cmptNames[k]<<")"<<endl;
	nonlinearRegression(data[k].col(ir+1), data[k].col(0), m);
	regressions[k].col(ir+1)=m.evaluateObjective(regressions[k].col(0));
	
	L(ir, 1+k)=m.lengthScale();
      }
      ir++;
    }
      
    for (int k=0; k<nk; k++)
    {
      gp[k]<<cmd[k].str()<<endl;
      for (int c=1; c<data[k].n_cols; c++)
      {
	arma::mat pdata;
	pdata=join_rows(data[k].col(0), data[k].col(c));
	gp[k].send1d( pdata );
	pdata=join_rows(regressions[k].col(0), regressions[k].col(c));
	gp[k].send1d( pdata );
      }
    }
  }
  
  {
    std::string chart_name=name_prefix+"_L_diag";
    std::string chart_file_name=chart_name+".png";

    Gnuplot gp;
    std::ostringstream cmd;
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'Radius [length]'; set ylabel 'L [length]'; set grid; ";
    cmd<<"plot 0 not lt -1";
    
    std::vector<double> ks=list_of<double>(0)(4)(8);
    
    BOOST_FOREACH(int k, ks)
    {
      cmd<<", '-' w lp lt "<<k+1<<" t 'L_"<<cmptNames[k]<<"'";
    }
    gp<<cmd.str()<<endl;
    BOOST_FOREACH(int k, ks)
    {
      arma::mat pdata;
      pdata=join_rows(L.col(0), L.col(k+1));
      gp.send1d(pdata);
    }
    
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Autocorrelation lengths", ""
    )));
  }

  {
    std::string chart_name=name_prefix+"_L_offdiag";
    std::string chart_file_name=chart_name+".png";

    Gnuplot gp;
    std::ostringstream cmd;
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'Radius [length]'; set ylabel 'L [length]'; set grid; ";
    cmd<<"plot 0 not lt -1";
    
    std::vector<double> ks=list_of<double>(1)(2)(3)(5)(6)(7);
    
    BOOST_FOREACH(int k, ks)
    {
      cmd<<", '-' w lp lt "<<k+1<<" t 'L_"<<cmptNames[k]<<"'";
    }
    gp<<cmd.str()<<endl;
    BOOST_FOREACH(int k, ks)
    {
      arma::mat pdata;
      pdata=join_rows(L.col(0), L.col(k+1));
      gp.send1d(pdata);
    }
    
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Cross-correlation lengths", ""
    )));
  }*/

}

}