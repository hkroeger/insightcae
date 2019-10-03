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

#include <boost/concept_check.hpp>

#include "base/linearalgebra.h"
#include "base/analysis.h"
#include "base/resultset.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "boost/format.hpp"

#include <QApplication>
#include <QMainWindow>

#include "qwt_plot.h"
#include "qwt_plot_grid.h"
#include "qwt_legend.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_scale_draw.h"
#include "qwt_column_symbol.h"

using namespace std;
using namespace insight;
using namespace boost;

void listContents(const ResultElementCollection& el, const std::string indent="")
{
  for (const auto& rel: el)
  {
    if (const auto* sub = dynamic_cast<const ResultElementCollection*>(rel.second.get()))
    {
      cout<<indent<<rel.first<<"/"<<endl;
      listContents(*sub, indent+"\t");
    }
    else {
      cout<<indent<<rel.first<<" ("<<rel.second->type()<<")"<<endl;
    }
  }
};


int main(int argc, char *argv[])
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    using namespace rapidxml;
    namespace po = boost::program_options;

    typedef std::vector<string> StringList;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "produce help message")
    ("libs", po::value< StringList >(),"Additional libraries with analysis modules to load")
    ("list,l", "List contents of result file")
    ("input-file,f", po::value< StringList >(),"Specifies input file(s).")
//    ("compareplot", po::value< string >(), "Compare plots. Specify path to plot. Optionally append name of curve with '_'.")
    ("comparescalar", po::value< string >(), "Compare scalar values. Specify path of scalar in result archive. "
                                             "Multiple scalars may plotted together: give list, separated by comma (without spaces). "
                                              "Put optional scale factor after variable name, separated by colon.")
    ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << endl;
        exit(-1);
    }

    if (!vm.count("input-file"))
    {
        cout<<"input file has to be specified!"<<endl;
        exit(-1);
    }

    try
    {

        if (vm.count("libs"))
        {
            StringList libs=vm["libs"].as<StringList>();
            for (const string& l: libs)
            {
                if (!boost::filesystem::exists(l))
                {
                    std::cerr << std::endl
                              << "Error: library file does not exist: "<<l
                              <<std::endl<<std::endl;
                    exit(-1);
                }
                loader.addLibrary(l);
            }
        }

        std::vector<string> fns=vm["input-file"].as<StringList>();
        std::vector<ResultElementCollection> r;

        // load results
        for (std::string fn: fns)
        {
          r.push_back(ResultElementCollection());

          cout<<"Reading results file "<<fn<<"..."<<flush;
          r.back().readFromFile(fn);
          cout<<"done."<<endl;

          if (vm.count("list"))
          {
            cout<<std::string(80, '=')<<endl<<endl;
            cout<<"Result file: "<<fn<<endl<<endl;
            listContents(r.back());
            cout<<endl<<std::string(80, '=')<<endl<<endl;
          }
        }

        if (vm.count("comparescalar"))
        {
          std::vector<string> varnames;
          boost::split(varnames, vm["comparescalar"].as<string>(), boost::is_any_of(","));
          if (varnames.size()<1)
            throw insight::Exception("At least one variable name has to be specified for comparison!");

          std::vector<double> sfs;
          for (size_t i=0; i< varnames.size(); i++)
          {
            std::vector<string> ns;
            boost::split(ns, varnames[i], boost::is_any_of(":"));
            if (ns.size()==1)
              sfs.push_back(1.0);
            else if (ns.size()==2)
            {
              varnames[i]=ns[0];
              sfs.push_back(to_number<double>(ns[1]));
            }
            else
              throw insight::Exception("Invalid syntax: "+vm["comparescalar"].as<string>());
          }

          typedef std::pair<string,std::vector<double> > NameAndValues;
          std::map<double,NameAndValues > sorted_vals;
          for (size_t i=0; i<r.size(); i++)
          {
            std::vector<double> vals;
            for (size_t j=0; j<varnames.size(); j++)
            {
              vals.push_back(r[i].getScalar(varnames[j])*sfs[j]);
            }
            sorted_vals[vals[0]] = NameAndValues(fns[i], vals);
          }
          for (const auto& v: sorted_vals)
          {
            cout<<v.second.first<<":\t";
            for (size_t i=0; i<varnames.size(); i++)
             cout <<"\t"<<varnames[i]<<" = "<<v.second.second[i]/sfs[i];
            cout<<endl;
          }

          QApplication app(argc, argv);

          QMainWindow mw;
          QwtPlot *plot=new QwtPlot();
          mw.setCentralWidget(plot);

          plot->insertLegend( new QwtLegend() );
          plot->setCanvasBackground( Qt::white );

          QwtPlotGrid *grid = new QwtPlotGrid();
          grid->attach(plot);

          QwtPlotMultiBarChart *d_barChartItem = new QwtPlotMultiBarChart( "Bar Chart " );
          d_barChartItem->setLayoutPolicy( QwtPlotMultiBarChart::AutoAdjustSamples );
          d_barChartItem->setSpacing( 20 );
          d_barChartItem->setMargin( 3 );

          d_barChartItem->attach( plot );


          for ( size_t i = 0; i < varnames.size(); i++ )
          {
              QwtColumnSymbol *symbol = new QwtColumnSymbol( QwtColumnSymbol::Box );
              // Die Konfiguration ist ähnlich der regulärer Widgets
              symbol->setLineWidth( 1 ); // Pixel-Dimension
              symbol->setFrameStyle( QwtColumnSymbol::Plain );
              symbol->setPalette( QPalette( QColor( rand()%255, rand()%255, rand()%255 ) ) );

              d_barChartItem->setSymbol( i, symbol );
          }

          QList<QwtText> titles;
          QVector< QVector<double> > values;
          QMap<double,QString> labels;
          int j=0;
          for (const auto& v: sorted_vals)
          {
            values.push_back( QVector<double>::fromStdVector(v.second.second) );
            labels[j++]=QString::fromStdString(v.second.first);
          }

          d_barChartItem->setBarTitles( titles );
          d_barChartItem->setSamples( values );

          class BarChartScaleDraw : public QwtScaleDraw
          {
          private:
            QMap<double,QString> *ids;
          public:
            BarChartScaleDraw(QMap<double,QString> *x)
              : ids(x)
            {
              enableComponent(QwtAbstractScaleDraw::Ticks, false);
              setLabelRotation(-90.);
              setLabelAlignment(Qt::AlignLeft);
            }

            virtual QwtText label(double v) const
            {
                    if (ids->contains(v))
                    {
                      QwtText t(ids->find(v).value());
                            return t;
                    }
                    else
                            return QwtText();
            }
          };

          string ylabel;
          for (size_t i=0; i<varnames.size(); i++)
          {
            if (i>0) ylabel+="\n";
            if ( fabs(sfs[i]-1.0)>1e-10) ylabel+=boost::str(boost::format("%g x ") % sfs[i]);
            ylabel+=varnames[i];
          }
          plot->setAxisTitle( QwtPlot::yLeft, QString::fromStdString(ylabel) );
          plot->setAxisScaleDraw(QwtPlot::xBottom, new BarChartScaleDraw(&labels));
          plot->setAxisScale(QwtPlot::xBottom, 0, j-1, 1);
          mw.show();
          return app.exec();
        }
    }




    catch (const std::exception& e)
    {
        insight::printException(e);
        return -1;
    }

    return 0;
}
