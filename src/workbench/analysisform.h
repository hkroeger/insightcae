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
#include <QPushButton>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPointer>
#include <QTableView>


#include "base/progressdisplayer/combinedprogressdisplayer.h"
#include "workbenchwindow.h"
#include "graphprogressdisplayer.h"

#include "iqdebugstream.h"
#include "logviewerwidget.h"
#include "remotedirselector.h"

#include "localrun.h"
#ifdef HAVE_WT
#include "remoterun.h"
#endif

#include "iqresultsetmodel.h"
#include "qactionprogressdisplayerwidget.h"


#include "iqsupplementedinputdatamodel.h"
#include "iqremoteparaviewdialog.h"
#include "iqparaviewdialog.h"
#include "iqexecutionworkspace.h"
#include "iqresultsetfiltermodel.h"
#include "iqresultsetdisplayerwidget.h"

#include <set>

#ifdef WIN32
#define WSL_DEFAULT
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






class AnalysisForm
: public QWidget, //QMdiSubWindow,
  public IQExecutionWorkspace
{
  Q_OBJECT

  friend class WorkbenchAction;
  friend class LocalRun;
  friend class RemoteRun;
  friend class WSLRun;
  friend class IQCaseDirectoryState;
  friend class IQWorkbenchRemoteExecutionState;
  
protected:

  // ====================================================================================
  // ======== Analysis-related members
  std::string analysisName_;
  bool isOpenFOAMAnalysis_;
  insight::supplementedInputDataBasePtr sid_;

//  insight::ResultSetPtr results_;
//  QPointer<insight::IQResultSetModel> resultsModel_;
//  insight::IQFilteredResultSetModel *filteredResultsModel_;
//  IQResultSetFilterModel *filterModel_;
  IQResultSetDisplayerWidget* resultsViewer_;
  IQSupplementedInputDataModel supplementedInputDataModel_;
  
  // ====================================================================================
  // ======== GUI widgets
  IQParameterSetModel *psmodel_;
  ParameterEditorWidget *peditor_;
  QTableView *sidtab_;
  IQDebugStream *cout_log_, *cerr_log_;
  LogViewerWidget *log_;

  QProgressBar* progressbar_;

  GraphProgressDisplayer *graphProgress_;
  insight::QActionProgressDisplayerWidget* actionProgress_;
  insight::CombinedProgressDisplayer progressDisplayer_;

  // ====================================================================================
  // ======== control elements
  QPushButton *save_log_btn_, *send_log_btn_, *clear_log_btn_, *auto_scroll_down_btn_;

  QPointer<QAction> act_save_, act_pack_;


  /**
   * @brief ist_file_
   * currently opened file
   */
  boost::filesystem::path ist_file_;

  /** if (checkAndUpdateWorkingDir(newDir, true))
              lastValidLocalWorkDirSetting_=newDir;
            else
              ui->leL
   * @brief pack_parameterset_
   * store preference for pack/not packing the parameter set during saving
   */
  bool pack_parameterset_;

  /**
   * @brief is_modified_
   * whether PS was modified since last save
   */
  bool is_modified_;

  std::set<insight::JobPtr> externalProcesses_;
  void cleanFinishedExternalProcesses();

  void connectLocalActions();
  void connectRemoteActions();

  void updateSaveMenuLabel();
  void updateWindowTitle();

  bool checkAnalysisExecutionPreconditions();


  // ====================================================================================
  // ======== current action objects


  QScopedPointer<WorkbenchAction> currentWorkbenchAction_;


  // ================================================================================
  // ================================================================================
  // ===== Status queries

  inline bool isRunningLocally() const
  {
    if (currentWorkbenchAction_)
      return (dynamic_cast<LocalRun*>(currentWorkbenchAction_.get()) != nullptr);
    return false;
  }

  inline bool isRunningRemotely() const
  {
#ifdef HAVE_WT
    if (currentWorkbenchAction_)
      return (dynamic_cast<RemoteRun*>(currentWorkbenchAction_.get()) != nullptr);
#endif
    return false;
  }

  inline bool isRunning() const
  {
    return isRunningLocally() || isRunningRemotely();
  }

  void downloadFromRemote( std::function<void()> completionCallback = [](){} );

public:
  AnalysisForm(
      QWidget* parent,
      const std::string& analysisName,
      bool logToConsole=false
      );
  ~AnalysisForm();
  
  const insight::ParameterSet& parameters() const;
  
  // ================================================================================
  // ================================================================================
  // ===== general logic

  WidgetWithDynamicMenuEntries* createMenus(QMenuBar* mainMenu);
//  void insertMenu(QMenuBar* mainMenu) override;
//  void removeMenu() override;

  void loadParameters(const boost::filesystem::path& fp);
  void saveParameters(bool *cancelled=nullptr);
  void saveParametersAs(bool *cancelled=nullptr);


  // ================================================================================
  // ================================================================================
  // ===== Remote run logic


  void startRemoteRun();
//  void disconnectFromRemoteRun();

  // ================================================================================
  // ================================================================================
  // ===== Local run logic

  void startLocalRun();

  bool isOpenFOAMAnalysis() const;
  insight::OperatingSystemSet compatibleOperatingSystems() const;


protected:
  void	closeEvent ( QCloseEvent * event ) override;

private Q_SLOTS:
  void onSaveParameters();
  void onSaveParametersAs();
  void onLoadParameters();

  void onRunAnalysis();
  void onKillAnalysis();

  // ================================================================================
  // ================================================================================
  // ===== error handling
  void onResultReady(insight::ResultSetPtr);
  void onAnalysisError(std::exception_ptr e);
  void onAnalysisCancelled();



  void onStartPV();
  void onStartPVLocal();
  void onStartPVRemote();

  void onCleanOFC();
  void onWnow();
  void onWnowAndStop();
  void onShell();

  void upload();
  void download();
  void resumeRemoteRun();

  void onShowParameterXML();

  void onConfigModification();

  void onUpdateSupplementedInputData(insight::supplementedInputDataBasePtr sid);


Q_SIGNALS:
//  void apply();
  void update();
  void statusMessage(const QString& message, int timeout=0);
  
private:

  QString lastWorkingDirectory_ = "", lastRemoteDirectory_ = "";
  Ui::AnalysisForm* ui;

};

#endif // ANALYSISFORM_H
