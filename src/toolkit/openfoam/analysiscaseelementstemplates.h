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

#include "analysiscaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/format.hpp"

#include "gnuplot-iostream.h"

namespace insight
{

template<class TPC, const char* TypeName>
TPCArray<TPC,TypeName>::TPCArray(OpenFOAMCase& cm, ParameterSet const &ps )
    : outputFilterFunctionObject(cm, ps),
      p_(ps)
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
        double r;
        if (p_.grading == Parameters::towardsEnd)
        {
            r = -::cos(M_PI*(0.5+0.5*x))*p_.R;
        }
        else if (p_.grading == Parameters::towardsStart)
        {
            r = (1.+::sin(-M_PI*(0.5+0.5*x)))*p_.R;
        }
        else if (p_.grading == Parameters::none)
        {
            r = x*p_.R;
        }

        cout<<"Creating tpc FOs for set "<<p_.name<<" at r="<<r<<endl;
        r_.push_back(r);

        tpc_tan_.push_back(new TPC(cm, getTanParameters(i)));
        tpc_ax_.push_back(new TPC(cm, getAxParameters(i)));
    }
}

template<class TPC, const char* TypeName>
OFDictData::dict TPCArray<TPC,TypeName>::functionObjectDict() const
{
    return OFDictData::dict();
}

template<class TPC, const char* TypeName>
void TPCArray<TPC,TypeName>::addIntoDictionaries(OFdicts& dictionaries) const
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



template<class TPC, const char* TypeName>
void TPCArray<TPC,TypeName>::evaluate(
    OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    ResultSetPtr& results,
    const std::string& shortDescription
) const
{
    Ordering so;
    evaluateSingle(cm, location, results,
                   p_.name+"_tan",
                   p_.tanSpan, axisTitleTan(),
                   tpc_tan_,
                   shortDescription+" (along tangential direction)",
                   so
                  );

    evaluateSingle(cm, location, results,
                   p_.name+"_ax",
                   p_.axSpan,  axisTitleAx(),
                   tpc_ax_,
                   shortDescription+" (along axial direction)", //"two-point correlation of velocity along axial direction at different radii"
                   so
                  );
}

template<class TPC, const char* TypeName>
void TPCArray<TPC,TypeName>::evaluateSingle
(
    OpenFOAMCase& cm, const boost::filesystem::path& location,
    ResultSetPtr& results,
    const std::string& name_prefix,
    double span,
    const std::string& axisLabel,
    const boost::ptr_vector<TPC>& tpcarray,
    const std::string& shortDescription,
    Ordering& so
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
                    arma::linspace<arma::mat>(0, span, p_.np),
                    res[k+1].row(res[k+1].n_rows-1).t() // one TPC profile per row (one row per output time step), get the last row
                )
            );
            data.col(1) /= data(0,1); // Normalize two-point correlation values (y)

            CorrelationFunctionModel m;
            cout<<"Fitting tpc profile for set "<<p_.name<<" at radius "<<ir<<" (r="<<r_[ir]<<"), component k="<<k<<" ("<<cmptNames[k]<<")"<<endl;
            nonlinearRegression(data.col(1), data.col(0), m);
            arma::mat regressiondata
            (
//                 arma::mat(
                join_rows
                (
                    data.col(0),
                    m.evaluateObjective(data.col(0))
                )
// #warning only half of rows is used
//                       ).rows(0, data.n_rows/2)
            );

            L(ir, 1+k)=m.lengthScale();

            tpc_curves[k].push_back
            (
                PlotCurve(data,
                          str( format("r%.2g")%r_[ir]),
                          "w p lt "+lexical_cast<std::string>(ir+1)+" t 'r="+str( format("%.2g")%r_[ir])+"'")
            );

            tpc_curves[k].push_back
            (
                PlotCurve(regressiondata,
                          str( format("r%.2gfit")%r_[ir]),
                          "w l lt "+lexical_cast<std::string>(ir+1)+" t 'r="+str( format("%.2g")%r_[ir])+" (fit)'")
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
            axisLabel, "$\\langle R_{"+std::string(cmptNames[k])+"} \\rangle$",
            tpc_curves[k],
            shortDescription+", two-point correlation function for component "+cmptNames[k]
        ).setOrder(so.next());
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
                    "L"+std::string(cmptNames[k]),
                    "w lp t '$L_{"+std::string(cmptNames[k])+"}$'"
                )
            );
        }

        addPlot
        (
            results, location, name_prefix+"_L_diag",
            "Radius [length]", "L [length]",
            L_diag_curves,
            shortDescription+", autocorrelation lengths"
        ).setOrder(so.next());
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
                    "L"+std::string(cmptNames[k]),
                    "w lp t '$L_{"+std::string(cmptNames[k])+"}$'"
                )
            );
        }

        addPlot
        (
            results, location, name_prefix+"_L_offdiag",
            "Radius [length]", "L [length]",
            L_offdiag_curves,
            shortDescription+", cross-correlation lengths"
        ).setOrder(so.next());
    }

}

}
