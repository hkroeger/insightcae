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

#include "toolkit_gui_export.h"


#ifndef Q_MOC_RUN
#include <base/analysis.h>
#endif

#include <map>
#include <vector>

#include <QWidget>
#include <QLabel>
#include <QMutex>
#include <QTabWidget>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>

class TOOLKIT_GUI_EXPORT GraphProgressChart
    : public QtCharts::QChartView
{
  Q_OBJECT

public:
  class LineSeriesData
  {
      const int maxRecent=100;

      QtCharts::QLineSeries* crv;

      QList<QPointF> values;

  public:
      LineSeriesData(const QString& name, QtCharts::QChart* chartData);
      ~LineSeriesData();

      void append(double x, double y);
      void updateLineSeries(int maxResolution=1000);
  };

  typedef std::map<std::string, std::shared_ptr<LineSeriesData> > CurveList;

protected:
  QtCharts::QChart* chartData_;

  size_t maxCnt_;
//  typedef std::map<std::string, std::vector<double> > ArrayList;
//  ArrayList progressX_;
//  ArrayList progressY_;
  CurveList curve_;
  bool needsRedraw_;
  QMutex mutex_;
  bool logscale_;

  double ymin_=1e10, ymax_=-1e10, xmin_=1e10, xmax_=-1e10;

public:
  GraphProgressChart(bool logscale, QWidget* parent=nullptr);
  ~GraphProgressChart();

  virtual void update(double t, const std::string& name, double y_value);
  virtual void reset();

public slots:
  void checkForUpdate();
};




class TOOLKIT_GUI_EXPORT GraphProgressDisplayer
: public QTabWidget,
  public insight::ProgressDisplayer
{
  Q_OBJECT
  
protected:
  std::map<std::string, GraphProgressChart*> charts_;

  void createChart(bool log, const std::string name);

public:
  GraphProgressDisplayer(QWidget* parent=nullptr);
  virtual ~GraphProgressDisplayer();

  GraphProgressChart* addChartIfNeeded(const std::string& name);

  virtual void reset();

  void update(const insight::ProgressState& pi) override;
  void logMessage(const std::string& line) override;
  void setActionProgressValue(const std::string& path, double value) override;
  void setMessageText(const std::string& path, const std::string& message) override;
  void finishActionProgress(const std::string& path) override;


};

#endif // GRAPHPROGRESSDISPLAYER_H
