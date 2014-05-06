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
    
    cout<<"Creating tpc FOs at r="<<r<<endl;
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
void TPCArray<TPC>::evaluate(OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results) const
{
  evaluateSingle(cm, location, results, 
		  p_.name_prefix()+"_tan", 
		  p_.tanSpan(), "Angle [rad]",
		  tpc_tan_, 
		  "two-point correlation of velocity along tangential direction at different radii"
		);
  
  evaluateSingle(cm, location, results, 
		  p_.name_prefix()+"_ax", 
		  p_.axSpan(),  "Axial distance [length]",
		  tpc_ax_, 
		  "two-point correlation of velocity along axial direction at different radii"
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
	cout<<"Fitting TPC for radius "<<ir<<" (r="<<r_[ir]<<"), component k="<<k<<" ("<<cmptNames[k]<<")"<<endl;
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
  }

}

}