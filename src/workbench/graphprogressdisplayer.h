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


#ifndef GRAPHPROGRESSDISPLAYER_H
#define GRAPHPROGRESSDISPLAYER_H

#ifndef Q_MOC_RUN
#include <base/analysis.h>
#endif

#include <map>
#include <vector>

#include <QWidget>
#include <QLabel>
#include <QMutex>

#include <qwt/qwt.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>

class GraphProgressDisplayer 
: public QwtPlot,
  public insight::ProgressDisplayer
{
  Q_OBJECT
  
protected:
  int maxCnt_;
  typedef std::map<std::string, std::vector<double> > ArrayList;
  ArrayList progressX_;
  ArrayList progressY_;
  std::map<std::string, QwtPlotCurve*> curve_;
  bool needsRedraw_;
  QMutex mutex_;
  
public:
    GraphProgressDisplayer(QWidget* parent=NULL);
    virtual ~GraphProgressDisplayer();

    virtual void reset();
    virtual void update(const insight::ProgressState& pi);
    
public slots:
  void checkForUpdate();
};

#endif // GRAPHPROGRESSDISPLAYER_H
