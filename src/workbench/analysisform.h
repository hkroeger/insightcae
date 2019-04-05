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
#include "boost/shared_ptr.hpp"
#endif

#include <QMdiSubWindow>
#include <QThread>
#include <QMetaType>
#include <QTreeWidget>
#include <QPushButton>
#include <QPlainTextEdit>

#include "workbench.h"
#include "graphprogressdisplayer.h"

#include "qdebugstream.h"



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
  
  std::shared_ptr<insight::Analysis> analysis_;
  
public:
  AnalysisWorker(const std::shared_ptr<insight::Analysis>& analysis);
  
public slots:
  void doWork(insight::ProgressDisplayer* pd=NULL);
  
signals:
  void resultReady(insight::ResultSetPtr);
  void finished();
};




class AnalysisForm
: public QMdiSubWindow,
  public workbench::WidgetWithDynamicMenuEntries
{
  Q_OBJECT
  
protected:
  std::string analysisName_;
  insight::ParameterSet parameters_;
  
  std::shared_ptr<insight::Analysis> analysis_;  
  insight::ResultSetPtr results_;
  
  GraphProgressDisplayer *progdisp_;
  QThread workerThread_;
  
  QTreeWidget *rt_;
  QTreeWidgetItem* rtroot_;

  insight::DirectoryParameter executionPathParameter_;
  ParameterEditorWidget* peditor_;
  
  Q_DebugStream *cout_log_, *cerr_log_;
  QPlainTextEdit *log_;
  
  QPushButton *save_log_btn_, *send_log_btn_, *clear_log_btn_, *auto_scroll_down_btn_;

  QMenu *menu_parameters_=nullptr, *menu_actions_=nullptr, *menu_results_=nullptr, *menu_tools_=nullptr, *menu_tools_of_=nullptr;
  QAction *act_param_show_=nullptr, *act_save_=nullptr, *act_save_as_=nullptr, *act_merge_=nullptr;
  QAction *act_run_=nullptr, *act_kill_=nullptr;
  QAction *act_save_rpt_=nullptr;
  QAction *act_tool_of_paraview_=nullptr, *act_tool_of_clean_=nullptr;

  /**
   * @brief ist_file_
   * currently opened file
   */
  boost::filesystem::path ist_file_;

  /**
   * @brief pack_parameterset_
   * store preference for pack/not packing the parameter set during saving
   */
  bool pack_parameterset_;

  /**
   * @brief is_modified_
   * whether PS was modified since last save
   */
  bool is_modified_;
  
public:
  AnalysisForm(QWidget* parent, const std::string& analysisName);
  ~AnalysisForm();
  
  inline insight::ParameterSet& parameters() { return parameters_; }
  inline insight::Analysis& analysis() { return *analysis_; }
  inline insight::DirectoryParameter& executionPathParameter() { return executionPathParameter_; }
  
  inline void forceUpdate() { emit update(); }

  virtual void insertMenu(QMenuBar* mainMenu);
  virtual void removeMenu();

  void loadParameters(const boost::filesystem::path& fp);
  void saveParameters(bool *cancelled=nullptr);
  void saveParametersAs(bool *cancelled=nullptr);

protected:
  virtual void	closeEvent ( QCloseEvent * event );

private Q_SLOTS:
  void onSaveParameters();
  void onSaveParametersAs();
  void onLoadParameters();
  void onRunAnalysis();
  void onKillAnalysis();
  void onResultReady(insight::ResultSetPtr);
  void onCreateReport();

  void onStartPV();
  void onCleanOFC();

  void saveLog();
  void sendLog();
  void clearLog();
  void autoScrollLog();
  void onShowParameterXML();

  void onConfigModification();

signals:
  void apply();
  void update();
  void runAnalysis(insight::ProgressDisplayer*);
  
private:
  Ui::AnalysisForm* ui;

};

#endif // ANALYSISFORM_H
