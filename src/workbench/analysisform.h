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

#ifndef ANALYSISFORM_H
#define ANALYSISFORM_H

#ifndef Q_MOC_RUN
#include "base/analysis.h"
#include "base/resultset.h"
#include "parametereditorwidget.h"
#endif

#include <QMdiSubWindow>
#include <QThread>
#include <QMetaType>
#include <QTreeWidget>

#include "boost/shared_ptr.hpp"

#include "graphprogressdisplayer.h"

namespace Ui
{
class AnalysisForm;
}

Q_DECLARE_METATYPE(insight::ParameterSet);

class AnalysisWorker
: public QObject
{
  Q_OBJECT
  QThread workerThread_;
  
  boost::shared_ptr<insight::Analysis> analysis_;
  
public:
  AnalysisWorker(const boost::shared_ptr<insight::Analysis>& analysis);
  
public slots:
  void doWork(insight::ProgressDisplayer* pd=NULL);
  
signals:
  void resultReady(insight::ResultSetPtr);
};

class AnalysisForm
: public QMdiSubWindow
{
  Q_OBJECT
  
protected:
  insight::ParameterSet parameters_;
  boost::shared_ptr<insight::Analysis> analysis_;
  insight::ResultSetPtr results_;
  GraphProgressDisplayer *progdisp_;
  QThread workerThread_;
  
  QTreeWidget *rt_;
  QTreeWidgetItem* rtroot_;

  ParameterEditorWidget* peditor_;
  
public:
  AnalysisForm(QWidget* parent, const std::string& analysisName);
  ~AnalysisForm();
  
  inline insight::ParameterSet& parameters() { return parameters_; }
  inline insight::Analysis& analysis() { return *analysis_; }
  
  inline void forceUpdate() { emit update(); }
    
private slots:
  void onSaveParameters();
  void onLoadParameters();
  void onRunAnalysis();
  void onKillAnalysis();
  void onResultReady(insight::ResultSetPtr);
  void onCreateReport();

signals:
  void apply();
  void update();
  void runAnalysis(insight::ProgressDisplayer*);
  
private:
  Ui::AnalysisForm* ui;

};

#endif // ANALYSISFORM_H
