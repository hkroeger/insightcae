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


#include "iqgraphprogressdisplayer.h"
#include "iqchartinteractivelegend.h"

#ifndef Q_MOC_RUN
#include "boost/foreach.hpp"
#endif


#include <QCoreApplication>

#include <QThread>
#include <QHBoxLayout>






using namespace insight;




void IQGraphProgressDisplayer::createChart(bool log, const std::string name)
{
    auto *chart=new IQGraphProgressChart(log, this);
    addTab(chart, QString::fromStdString(name));
    charts_[name] = chart;
}




IQGraphProgressDisplayer::IQGraphProgressDisplayer(QWidget* parent)
: QTabWidget(parent)
{}




IQGraphProgressDisplayer::~IQGraphProgressDisplayer()
{}





IQGraphProgressChart* IQGraphProgressDisplayer::addChartIfNeeded(const std::string& name)
{
  std::vector<std::string> names;
  boost::split(names, name, boost::is_any_of("/"));

  bool log;
  if ( boost::algorithm::ends_with(names.back(), "residual") )
  {
    log=true;
  }
  else
  {
    log=false;
  }

  decltype(charts_)::iterator c;
  if ( (c=charts_.find(name))==charts_.end())
  {

    createChart(log, name);
    if ( (c=charts_.find(name))!=charts_.end() )
      return c->second;
    else
      throw insight::Exception("Failed to create chart "+name+"!");
  }
  else
  {
    return c->second;
  }
}




void IQGraphProgressDisplayer::reset()
{
  for (auto& c: charts_)
  {
    c.second->deleteLater();
  }
  charts_.clear();
}



void IQGraphProgressDisplayer::update(const insight::ProgressState& pi)
{
  QMetaObject::invokeMethod(  // post into GUI thread as this method might be called from different thread
        qApp,
        [this,pi]()
        {
          double t=pi.first;
          const ProgressVariableList& pvl=pi.second;

          for ( const ProgressVariableList::value_type& i: pvl)
          {
            const std::string& name = i.first;
            double y_value = i.second;

            std::vector<std::string> np;
            boost::split(np, name, boost::is_any_of("/"));

            if (np.size()==1)
            {
              auto* c = addChartIfNeeded("Progress");
              c->update(t, np[0], y_value);
            }
            else if (np.size()==2)
            {
              auto* c = addChartIfNeeded(np[0]);
              c->update(t, np[1], y_value);
            }
            else if (np.size()>2)
            {
              std::string ln=*np.rbegin();
              np.erase(np.end()-1);
              std::string pn = boost::algorithm::join(np, "/");

              auto* c = addChartIfNeeded(pn);
              c->update(t, ln, y_value);
            }
          }
        }
  );
}




void IQGraphProgressDisplayer::logMessage(const std::string &line)
{}




void IQGraphProgressDisplayer::setActionProgressValue(const std::string&, double)
{}




void IQGraphProgressDisplayer::setMessageText(const std::string&, const std::string&)
{}




void IQGraphProgressDisplayer::finishActionProgress(const std::string&)
{}
