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
#include "base/remoteexecution.h"
#include "parametereditorwidget.h"
#endif

#include <QMdiSubWindow>
#include <QThread>
#include <QMetaType>
#include <QTreeWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QProgressBar>

#include "workbench.h"
#include "graphprogressdisplayer.h"

#include "qdebugstream.h"
#include "logviewerwidget.h"
#include "remotedirselector.h"

#include "localrun.h"
#ifdef HAVE_WT
#include "remoterun.h"
#endif

namespace Ui
{
class AnalysisForm;
}


namespace insight
{
class TaskSpoolerInterface;
class SolverOutputAnalyzer;
}

class WorkbenchAction;


class AnalysisForm
: public QMdiSubWindow,
  public workbench::WidgetWithDynamicMenuEntries
{
  Q_OBJECT

  friend class WorkbenchAction;
  friend class LocalRun;
  friend class RemoteRun;
  
protected:

  // ====================================================================================
  // ======== Analysis-related members
  std::string analysisName_;
  bool isOpenFOAMAnalysis_;
  insight::ParameterSet parameters_;
  insight::ResultSetPtr results_;
  
  // ====================================================================================
  // ======== GUI widgets
  GraphProgressDisplayer *progdisp_;
  QTreeWidget *rt_;
  QTreeWidgetItem* rtroot_;
  ParameterEditorWidget* peditor_;
  Q_DebugStream *cout_log_, *cerr_log_;
  LogViewerWidget *log_;
  QProgressBar* progressbar_;

  // ====================================================================================
  // ======== control elements
  QPushButton *save_log_btn_, *send_log_btn_, *clear_log_btn_, *auto_scroll_down_btn_;

  QMenu *menu_parameters_=nullptr, *menu_actions_=nullptr, *menu_results_=nullptr, *menu_tools_=nullptr, *menu_tools_of_=nullptr;
  QAction *act_param_show_=nullptr, *act_save_=nullptr, *act_save_as_=nullptr, *act_pack_=nullptr, *act_merge_=nullptr;
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

  void updateSaveMenuLabel();
  void updateWindowTitle();

  insight::RemoteServerInfo lookupRemoteServerByLabel(const QString& hostLabel) const;

  bool checkAnalysisExecutionPreconditions();
  bool changeWorkingDirectory(const QString& wd);
  bool changeRemoteLocation(const QString& hostLabel, const QString& remoteDir);
  bool changeRemoteLocation(const insight::RemoteExecutionConfig* rec = nullptr);
  void applyDirectorySettings();

  // ====================================================================================
  // ======== current action objects
  std::unique_ptr<insight::CaseDirectory> caseDirectory_;
  std::unique_ptr<insight::RemoteExecutionConfig> remoteDirectory_;
  std::unique_ptr<WorkbenchAction> currentWorkbenchAction_;

  // ================================================================================
  // ================================================================================
  // ===== Status queries

  inline bool isRunningLocally() const
  {
    if (currentWorkbenchAction_)
      return dynamic_cast<LocalRun*>(currentWorkbenchAction_.get());
    return false;
  }

  inline bool isRunningRemotely() const
  {
#ifdef HAVE_WT
    if (currentWorkbenchAction_)
      return dynamic_cast<RemoteRun*>(currentWorkbenchAction_.get());
#endif
    return false;
  }

  inline bool isRunning() const
  {
    return isRunningLocally() || isRunningRemotely();
  }

public:
  AnalysisForm(QWidget* parent, const std::string& analysisName, const boost::filesystem::path& workingDirectory="");
  ~AnalysisForm();
  
  inline insight::ParameterSet& parameters() { return parameters_; }
  
  // ================================================================================
  // ================================================================================
  // ===== general logic

  void insertMenu(QMenuBar* mainMenu) override;
  void removeMenu() override;

  void loadParameters(const boost::filesystem::path& fp);
  void saveParameters(bool *cancelled=nullptr);
  void saveParametersAs(bool *cancelled=nullptr);


  // ================================================================================
  // ================================================================================
  // ===== Remote run logic


  void startRemoteRun();
  void resumeRemoteRun();
  void disconnectFromRemoteRun();

  // ================================================================================
  // ================================================================================
  // ===== Local run logic

  void startLocalRun();


protected:
  void	closeEvent ( QCloseEvent * event ) override;

private Q_SLOTS:
  void onSaveParameters();
  void onSaveParametersAs();
  void onLoadParameters();

  void workingDirectoryInputChanged();

  void onRunAnalysis();
  void onKillAnalysis();
  void onAnalysisKilled();

  // ================================================================================
  // ================================================================================
  // ===== error handling
  void onAnalysisErrorOccurred(insight::Exception e);
  void onAnalysisWarningOccurred(insight::Exception e);

  void onResultReady(insight::ResultSetPtr);

  void onCreateReport();

  void onStartPV();
  void onCleanOFC();
  void onWnow();
  void onWnowAndStop();
  void onShell();

  void upload();
  void download();

  void onShowParameterXML();

  void onConfigModification();

Q_SIGNALS:
  void apply();
  void update();
  void statusMessage(const QString& message, int timeout=0);
  
private:

  QString lastWorkingDirectory_ = "", lastRemoteDirectory_ = "";
  Ui::AnalysisForm* ui;

};

#endif // ANALYSISFORM_H
