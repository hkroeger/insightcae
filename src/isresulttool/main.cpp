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
#include "base/resultelements/chart.h"

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

#include "resultviewwindow.h"

#include <QtCharts>

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


const std::vector<unsigned int> colorTable = {
  0xe6194b,
  0x3cb44b,
  0xffe119,
  0x0082c8,
  0xf58231,
  0x911eb4,
  0x46f0f0,
  0xf032e6,
  0xd2f53c,
  0xfabebe,
  0x008080,
  0xe6beff,
  0xaa6e28,
  0xfffac8,
  0x800000,
  0xaaffc3,
  0x808000,
  0xffd8b1,
  0x000080,
  0x808080,
  0xffffff,
  0x000000
};

QColor colorValue(int crvIndex)
{
  auto cv=colorTable[ crvIndex%colorTable.size() ];
  return QColor( (cv&0xff0000)>>16, (cv&0xff00)>>8, cv&0xff );
}

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
    ("display,d", "Display each result file in separate window")
    ("input-file,f", po::value< StringList >(),"Specifies input file(s).")
    ("compareplot", po::value< string >(), "Compare plots. Specify path to plot. Append name of the curve with ':'.")
    ("comparescalar", po::value< string >(), "Compare scalar values. Specify path of scalar in result archive. "
                                             "Multiple scalars may plotted together: give list, separated by comma (without spaces). "
                                              "Put optional scale factor after variable name, separated by colon.")
    ("sort,s", "sort entries in comparison")
    ("render", "Render into PDF")
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

    bool someActionDone=false;
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
        std::vector<ResultSetPtr> results;



        // load results
        for (std::string fn: fns)
        {
          boost::filesystem::path inpath(fn);

          insight::assertion( boost::filesystem::exists(inpath),
                              "input file "+inpath.string()+" does not exist!" );

          cout<<"Reading results file "<<inpath<<"..."<<flush;
          results.push_back(ResultSetPtr(new ResultSet(inpath)));
          cout<<"done."<<endl;

          if (vm.count("list"))
          {
            cout<<std::string(80, '=')<<endl<<endl;
            cout<<"Result file: "<<inpath<<endl<<endl;
            listContents(*results.back());
            cout<<endl<<std::string(80, '=')<<endl<<endl;
          }

          if (vm.count("render"))
          {
            boost::filesystem::path outpath =
                inpath.parent_path() / (inpath.filename().stem().string()+".pdf");
            results.back()->generatePDF( outpath );
          }
        }



        if (vm.count("comparescalar"))
        {
          someActionDone=true;
          std::vector<string> varnames;
          boost::split(varnames, vm["comparescalar"].as<string>(), boost::is_any_of(","));
          if (varnames.size()<1)
            throw insight::Exception("At least one variable name has to be specified for comparison!");

          // store scale factors, default to 1
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

          // sort files by value of first scalar
          typedef std::pair<string,std::vector<double> > NameAndValues;
          std::vector<NameAndValues > sorted_vals;
          for (size_t i=0; i<results.size(); i++)
          {
            std::vector<double> vals;
            for (size_t j=0; j<varnames.size(); j++)
            {
              vals.push_back(results[i]->getScalar(varnames[j])*sfs[j]);
            }
            sorted_vals.push_back(NameAndValues(fns[i], vals));
          }

          if (vm.count("sorted"))
          {
            sort(sorted_vals.begin(), sorted_vals.end(),
                [](const NameAndValues& v1, const NameAndValues& v2)
                {
                  return v1.second[0]<v2.second[0];
                }
            );
          }

          // output
          for (const auto& v: sorted_vals)
          {
            cout<<v.first<<":\t";
            for (size_t i=0; i<varnames.size(); i++)
             cout <<"\t"<<varnames[i]<<" = "<<v.second[i]/sfs[i];
            cout<<endl;
          }

          QApplication app(argc, argv);

          QMainWindow mw;
          auto chartData = new QChart;
          auto chart=new QChartView;
          chart->setChart(chartData);
          mw.setCentralWidget(chart);


          QStringList categories;
          QBarSeries *series = new QBarSeries();
          for ( size_t i = 0; i < varnames.size(); i++ )
          {
            QString label;
            if ( fabs(sfs[i]-1.0)>1e-10) label+=QString("%1 x ").arg(sfs[i]);
            label+=QString::fromStdString(varnames[i]);
            auto bs = new QBarSet(label);

            //bs->setColor( QColor( rand()%255, rand()%255, rand()%255 ) );
            bs->setColor(colorValue(i));
            for (const auto& v: sorted_vals) // through all files
            {
              bs->append( v.second[i] );
              categories.append(QString::fromStdString(v.first));
            }
            series->append(bs);
          }
          chartData->addSeries(series);

          QBarCategoryAxis *axisX = new QBarCategoryAxis();
          axisX->append(categories);
          chartData->addAxis(axisX, Qt::AlignBottom);
          series->attachAxis(axisX);

          QValueAxis *axisY = new QValueAxis();
          QString ylabel;
          for (size_t i=0; i<varnames.size(); i++)
          {
            if (i>0) ylabel+="<br>";
            if ( fabs(sfs[i]-1.0)>1e-10) ylabel+=QString("%1 x ").arg(sfs[i]);
            ylabel+=QString::fromStdString(varnames[i]);
          }
          axisY->setTitleText(ylabel);
          chartData->addAxis(axisY, Qt::AlignLeft);
          series->attachAxis(axisY);

          chartData->legend()->setVisible(true);
          chartData->legend()->setAlignment(Qt::AlignBottom);

          mw.show();
          return app.exec();
        }

        if (vm.count("compareplot"))
        {
          someActionDone=true;
          std::vector<string> chartAndCurveNames;
          boost::split(chartAndCurveNames, vm["compareplot"].as<string>(), boost::is_any_of(","));
          if (chartAndCurveNames.size()<1)
            throw insight::Exception("At least one variable name has to be specified for comparison!");


          for (size_t j=0; j<chartAndCurveNames.size(); j++)
          {

            std::vector<PlotCurve> curves;
            QString xlabel, ylabel;
            for (size_t i=0; i<results.size(); i++)
            {
              std::vector<string> chart_curve;
              boost::split(chart_curve, chartAndCurveNames[j], boost::is_any_of(":"));
              insight::assertion( chart_curve.size()==2,
                                  "a curve name must be specified after each chart name, separated by colon" );

              const auto& chart = results[i]->get<Chart>(chart_curve[0]);

              auto crv = std::find_if( chart.chartData()->plc_.begin(), chart.chartData()->plc_.end(),
                                       [&](const PlotCurve& pc) { return pc.plaintextlabel()==chart_curve[1]; } );
              insight::assertion( crv!=chart.chartData()->plc_.end(),
                                  "no curve of name "+chart_curve[1]+" found in chart "+chart_curve[0]+"!" );

              curves.push_back( *crv );

              if (xlabel.isEmpty())
                xlabel=QString::fromStdString(chart.chartData()->xlabel_);
              if (ylabel.isEmpty())
                ylabel=QString::fromStdString(chart.chartData()->ylabel_);
            }

            QApplication app(argc, argv);

            auto chartData = new QChart;
            chartData->legend()->setVisible(true);
            chartData->legend()->setAlignment(Qt::AlignBottom);

            for ( size_t i = 0; i < curves.size(); i++ )
            {
              auto *series = new QLineSeries();

              auto label = QString::fromStdString(fns[i]);
              series->setName(label);

              series->setPen(QPen(QBrush(colorValue(i)), 2));
              for (arma::uword j=0; j<curves[i].xy().n_rows; j++)
              {
                series->append( curves[i].xy()(j,0), curves[i].xy()(j,1) );
              }
              chartData->addSeries(series);
            }
            chartData->createDefaultAxes();
            chartData->axes(Qt::Horizontal)[0]->setTitleText(xlabel);
            chartData->axes(Qt::Vertical)[0]->setTitleText(ylabel);

            QMainWindow mw;
            auto chartView=new QChartView(chartData);
            chartView->setRenderHint(QPainter::Antialiasing);
            mw.setCentralWidget(chartView);
            mw.resize(800, 600);
            mw.show();
            return app.exec();
          }
        }

        if (vm.count("display") || !someActionDone)
        {
          QApplication app(argc, argv);
          for (size_t i=0; i<results.size(); i++)
          {
            auto& cr=results[i];
            auto w=new ResultViewWindow(cr);
            w->setWindowTitle(w->windowTitle()+" - "+QString::fromStdString(fns[i]));
            w->show();
          }
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
