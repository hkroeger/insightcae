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
//  *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifdef HAVE_WT
#include "remoterun.h"
#endif
#include "localrun.h"

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#endif

#include "base/analysis.h"
#include "base/remoteserverlist.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"

#include "analysisform.h"
#include "ui_analysisform.h"
//#include "parameterwrapper.h"
#include "qresultsetmodel.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QScrollBar>
#include <QStatusBar>
#include <QSettings>
#include <QProcess>
#include <QCheckBox>
#include "email.h"

#include "ui_xml_display.h"

#include <cstdlib>
#include <memory>

#include "of_clean_case.h"
#include "remotesync.h"
#include "base/remoteserverlist.h"
#include "remotedirselector.h"


namespace fs = boost::filesystem;




void QCaseDirectoryState::setAFEnabledState(bool enabled)
{
  insight::dbg()<<"set QCaseDirectoryState to enabled="
                <<enabled<<std::endl;
  if (enabled)
    af_->ui->lblWorkingDirectory->setText( *this );
  else
    af_->ui->lblWorkingDirectory->setText("(unset)");

  af_->ui->btnParaview->setEnabled(enabled);
  af_->ui->btnClean->setEnabled(enabled);
  af_->ui->btnShell->setEnabled(enabled);
}




QCaseDirectoryState::QCaseDirectoryState(AnalysisForm *af, const boost::filesystem::path& path, bool keep)
  : insight::CaseDirectory(path, keep),
    af_(af)
{
  setAFEnabledState(true);
}




QCaseDirectoryState::QCaseDirectoryState(AnalysisForm *af, bool keep, const boost::filesystem::path& prefix)
  : insight::CaseDirectory(keep, prefix),
    af_(af)
{
  setAFEnabledState(true);
}




QCaseDirectoryState::~QCaseDirectoryState()
{
  setAFEnabledState(false);
}


QCaseDirectoryState::operator QString() const
{
  return QString::fromStdString( string() );
}




void QRemoteExecutionState::setAFEnabledState(bool enabled)
{
  insight::dbg()<<"set QRemoteExecutionState to enabled="
                <<enabled<<std::endl;

  af_->ui->lblHostName->setEnabled(enabled);
  if (enabled)
    af_->ui->lblHostName->setText(
          QString::fromStdString(server()->serverLabel()) );
  else
    af_->ui->lblHostName->setText("(none)");

  af_->ui->lblRemoteDirectory->setEnabled(enabled);
  if (enabled)
    af_->ui->lblRemoteDirectory->setText(
          QString::fromStdString(remoteDir().string()) );
  else
    af_->ui->lblRemoteDirectory->setText("(none)");

  af_->ui->btnDisconnect->setEnabled(enabled);
  af_->ui->btnResume->setEnabled(enabled);
  af_->ui->btnUpload->setEnabled(enabled);
  af_->ui->btnDownload->setEnabled(enabled);
  af_->ui->btnRemoveRemote->setEnabled(enabled);
  af_->ui->lblRemote_1->setEnabled(enabled);
  af_->ui->lblRemote_2->setEnabled(enabled);
}

QRemoteExecutionState::QRemoteExecutionState(
    AnalysisForm *af,
    const boost::filesystem::path& location,
    const RemoteLocation &remoteLocation )
  : insight::RemoteExecutionConfig(location, remoteLocation),
    af_(af)
{
  setAFEnabledState(true);
}


QRemoteExecutionState::QRemoteExecutionState(
    AnalysisForm *af,
    const boost::filesystem::path& location,
    const boost::filesystem::path& localREConfigFile
    )
  : insight::RemoteExecutionConfig(location, localREConfigFile),
    af_(af)
{
  setAFEnabledState(true);
}


QRemoteExecutionState::QRemoteExecutionState(
    AnalysisForm *af,
    insight::RemoteServer::ConfigPtr rsc,
    const boost::filesystem::path& location,
    const boost::filesystem::path& remotePath,
    const boost::filesystem::path& localREConfigFile
    )
  : insight::RemoteExecutionConfig(rsc, location, remotePath, localREConfigFile),
    af_(af)
{
  setAFEnabledState(true);
}

QRemoteExecutionState::~QRemoteExecutionState()
{
  setAFEnabledState(false);
}





AnalysisForm::AnalysisForm(
    QWidget* parent,
    const std::string& analysisName,
    bool logToConsole
    )
: QMdiSubWindow(parent),
  analysisName_(analysisName),
  isOpenFOAMAnalysis_(false),
  pack_parameterset_(true),
  is_modified_(false)
{
    // load default parameters
    auto defaultParams = insight::Analysis::defaultParameters(analysisName_);
//    parameters_ = defaultParams;

    {
      insight::AnalysisPtr a( insight::Analysis::lookup(analysisName_, defaultParams, "") );
      isOpenFOAMAnalysis_ = bool( std::dynamic_pointer_cast<insight::OpenFOAMAnalysis>( a ) );
    }

    ui = new Ui::AnalysisForm;
    QWidget* iw=new QWidget(this);
    ui->setupUi(iw);
    setWidget(iw);

    if (isOpenFOAMAnalysis_)
    {
      ui->btnParaview->setEnabled(true);
      connect( ui->btnParaview, &QPushButton::clicked, this, &AnalysisForm::onStartPV );
      ui->btnClean->setEnabled(true);
      connect( ui->btnClean, &QPushButton::clicked, this, &AnalysisForm::onCleanOFC );
      ui->btnWriteNow->setEnabled(true);
      connect( ui->btnWriteNow, &QPushButton::clicked, this, &AnalysisForm::onWnow );
      ui->btnWriteNowAndStop->setEnabled(true);
      connect( ui->btnWriteNowAndStop, &QPushButton::clicked, this, &AnalysisForm::onWnowAndStop );
    }

    connect( ui->btnShell, &QPushButton::clicked,
             this, &AnalysisForm::onShell);

    graphProgress_=new GraphProgressDisplayer;
    actionProgress_=new insight::QActionProgressDisplayerWidget;
    progressDisplayer_.add(graphProgress_);
    progressDisplayer_.add(actionProgress_);

    QSplitter* spl=new QSplitter(Qt::Vertical);
    QWidget* lower = new QWidget;
    QHBoxLayout* hbl = new QHBoxLayout(lower);
    spl->addWidget(graphProgress_);
    spl->addWidget(lower);
    log_=new LogViewerWidget(spl);
    hbl->addWidget(log_);

    QVBoxLayout* vbl=new QVBoxLayout;
    hbl->addLayout(vbl);
    save_log_btn_=new QPushButton("Save...");
    send_log_btn_=new QPushButton("Email...");
    clear_log_btn_=new QPushButton("Clear");
    auto_scroll_down_btn_=new QPushButton("Auto Scroll");
    connect(save_log_btn_, &QPushButton::clicked, log_, &LogViewerWidget::saveLog);
    connect(send_log_btn_, &QPushButton::clicked, log_, &LogViewerWidget::sendLog);
    connect(clear_log_btn_, &QPushButton::clicked, log_, &LogViewerWidget::clearLog);
    connect(auto_scroll_down_btn_, &QPushButton::clicked, log_, &LogViewerWidget::autoScrollLog);
    vbl->addWidget(save_log_btn_);
    vbl->addWidget(send_log_btn_);
    vbl->addWidget(clear_log_btn_);
    vbl->addWidget(auto_scroll_down_btn_);
    vbl->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Minimum,QSizePolicy::Expanding));
    ui->runTabLayout->addWidget(spl);
    //ui->runTabLayout->addWidget(actionProgress_);
    ui->verticalLayout_5->addWidget(actionProgress_);

//    ui->verticalLayout->addWidget(actionProgress_);
    

    if (!logToConsole)
    {
      cout_log_ = new Q_DebugStream(std::cout);
      connect(cout_log_, &Q_DebugStream::appendText, log_, &LogViewerWidget::appendDimmedLine);
      cerr_log_ = new Q_DebugStream(std::cerr);
      connect(cerr_log_, &Q_DebugStream::appendText, log_, &LogViewerWidget::appendErrorLine);
    }

    updateWindowTitle();
    connect(ui->btnRun, &QPushButton::clicked, this, &AnalysisForm::onRunAnalysis);
    connect(ui->btnKill, &QPushButton::clicked, this, &AnalysisForm::onKillAnalysis);


    insight::ParameterSet_VisualizerPtr viz;
    insight::ParameterSet_ValidatorPtr vali;

    try {
        viz = insight::Analysis::visualizer(analysisName_);
        viz ->setProgressDisplayer(&progressDisplayer_);
    } catch (const std::exception& e)
    {
      /* ignore, if non-existent */
      std::cout<<"Info: no visualizer for \""<<analysisName_<<"\" available."<<std::endl;
    }

    try {
        vali = insight::Analysis::validator(analysisName_);
    } catch (const std::exception& e)
    { /* ignore, if non-existent */ }

    peditor_=new ParameterEditorWidget(/*parameters_*/defaultParams, defaultParams, ui->inputTab, viz, vali);
    ui->inputTabLayout->addWidget(peditor_);

//    QObject::connect(this, &AnalysisForm::apply, peditor_, &ParameterEditorWidget::onApply);
//    QObject::connect(this, &AnalysisForm::update, peditor_, &ParameterEditorWidget::onUpdate);

    connect(peditor_, &ParameterEditorWidget::parameterSetChanged,
            this, &AnalysisForm::onConfigModification);

    QSettings settings("silentdynamics", "workbench");

    if (peditor_->hasVisualizer())
      peditor_->restoreState(settings.value("parameterEditor_wViz").toByteArray());
    else
      peditor_->restoreState(settings.value("parameterEditor_woViz").toByteArray());

    pack_parameterset_ = settings.value("pack_parameterset", QVariant(true)).toBool();

    connectLocalActions();
    connectRemoteActions();

    progressbar_=new QProgressBar(this);
    auto *sb = new QStatusBar(this);
    this->layout()->addWidget(sb);
    sb->addPermanentWidget(progressbar_);
    connect(this, &AnalysisForm::statusMessage,
            sb, &QStatusBar::showMessage);



//    connect(ui->gbExecuteOnRemoteHost, &QGroupBox::toggled,
//            [&](bool checked)
//            {
//              if (checked && isRunningLocally())
//              {
//                auto answer= QMessageBox::critical(this,
//                                      "Attention",
//                                      "There is a local analysis running. It has to be terminated, before any remote analysis can be managed.\n"
//                                      "You might consider to gracefully stop the simulation by triggering \"Write now+stop\" before switching to remote execution.\n"
//                                      "\nKill local analysis?",
//                                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
//                                      QMessageBox::No);
//                if (answer==QMessageBox::Yes)
//                {
//                  onKillAnalysis();
//                }
//                else
//                {
//                  ui->gbExecuteOnRemoteHost->setChecked(false);
//                }
//              }
//              if (!checked && isRunningRemotely())
//              {
//                auto answer= QMessageBox::critical(this,
//                                      "Attention",
//                                      "There is a remote analysis running. It has to be disconnected, before any local analysis can be managed.\n"
//                                      "The remote analysis will continue and you can reconnect at any time.\n"
//                                      "\nDisconnect from remote analysis?",
//                                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
//                                      QMessageBox::Yes);
//                if (answer==QMessageBox::Yes)
//                {
//                  disconnectFromRemoteRun();
//                }
//                else
//                {
//                  ui->gbExecuteOnRemoteHost->setChecked(true);
//                }
//              }
//              if (!isRunning())
//              {
//                if (checked && !remoteDirectory_)
//                {
//                  changeRemoteLocation(ui->ddlExecutionHosts->currentText(), ui->leRemoteDirectory->text());
//                }
//                if (!checked && remoteDirectory_)
//                {
//                  remoteDirectory_.reset();
//                }
//              }
//            }
//    );

    insight::connectToCWithContentsDisplay(ui->resultsToC, ui->resultElementDetails);

#ifndef HAVE_WT
    ui->gbExecuteOnRemoteHost->setChecked(false);
    ui->gbExecuteOnRemoteHost->setEnabled(false);
#endif

}



const insight::ParameterSet& AnalysisForm::parameters() const
{
  return peditor_->model()->getParameterSet();
}



AnalysisForm::~AnalysisForm()
{
  currentWorkbenchAction_.reset();
  delete ui;
}





void AnalysisForm::insertMenu(QMenuBar* mainMenu)
{
    workbench::WidgetWithDynamicMenuEntries::insertMenu(mainMenu);

    menu_parameters_=mainMenu_->addMenu("&Parameters");

    if (!act_save_) act_save_=new QAction("&S", this);
    //updateSaveMenuLabel();  // moved to below..
    act_save_->setShortcut(Qt::CTRL + Qt::Key_S);
    menu_parameters_->addAction( act_save_ );
    connect( act_save_, &QAction::triggered,
             this, &AnalysisForm::onSaveParameters );

    if (!act_save_as_) act_save_as_=new QAction("&Save parameter set as...", this);
    menu_parameters_->addAction( act_save_as_ );
    connect( act_save_as_, &QAction::triggered,
             this, &AnalysisForm::onSaveParametersAs );

    if (!act_pack_) act_pack_=new QAction("&Pack external files into parameter file", this);
    act_pack_->setCheckable(true);

    menu_parameters_->addAction( act_pack_ );
    connect( act_pack_, &QAction::triggered,
             [&]()
             {
               pack_parameterset_ = act_pack_->isChecked();
               updateSaveMenuLabel();
             }
    );

    updateSaveMenuLabel();

    if (!act_merge_) act_merge_=new QAction("&Merge other parameter set into current...", this);
    menu_parameters_->addAction( act_merge_ );
    connect( act_merge_, &QAction::triggered, this, &AnalysisForm::onLoadParameters );

    if (!act_param_show_) act_param_show_=new QAction("&Show in XML format", this);
    menu_parameters_->addAction( act_param_show_ );
    connect( act_param_show_, &QAction::triggered, this, &AnalysisForm::onShowParameterXML );


    menu_actions_=mainMenu_->addMenu("&Actions");

    if (!act_run_) act_run_=new QAction("&Run Analysis", this);
    menu_actions_->addAction( act_run_ );
    connect( act_run_, &QAction::triggered, this, &AnalysisForm::onRunAnalysis );
    if (!act_kill_) act_kill_=new QAction("&Stop Analysis", this);
    menu_actions_->addAction( act_kill_ );
    connect( act_kill_, &QAction::triggered, this, &AnalysisForm::onKillAnalysis );

    menu_results_=mainMenu_->addMenu("&Results");

    if (!act_save_rpt_) act_save_rpt_=new QAction("Create &report...", this);
    menu_results_->addAction( act_save_rpt_ );
    connect( act_save_rpt_, &QAction::triggered, this, &AnalysisForm::onCreateReport );

    menu_tools_=mainMenu_->addMenu("&Tools");
    menu_tools_of_=menu_tools_->addMenu("&OpenFOAM");
    if (!act_tool_of_paraview_) act_tool_of_paraview_=new QAction("Start ParaView in execution directory", this);
    menu_tools_of_->addAction( act_tool_of_paraview_ );
    connect( act_tool_of_paraview_, &QAction::triggered, this, &AnalysisForm::onStartPV );
    if (!act_tool_of_clean_) act_tool_of_clean_=new QAction("Clean OpenFOAM case...", this);
    menu_tools_of_->addAction( act_tool_of_clean_ );
    connect( act_tool_of_clean_, &QAction::triggered, this, &AnalysisForm::onCleanOFC );
}




void AnalysisForm::removeMenu()
{
    if (mainMenu_)
    {
        menu_parameters_->removeAction(act_save_); act_save_->disconnect();
        menu_parameters_->removeAction(act_save_as_); act_save_as_->disconnect();
        menu_parameters_->removeAction(act_merge_); act_merge_->disconnect();
        menu_parameters_->removeAction(act_param_show_); act_param_show_->disconnect();

        menu_actions_->removeAction(act_run_); act_run_->disconnect();
        menu_actions_->removeAction(act_kill_); act_kill_->disconnect();

        menu_results_->removeAction(act_save_rpt_); act_save_rpt_->disconnect();

        menu_tools_of_->removeAction(act_tool_of_paraview_); act_tool_of_paraview_->disconnect();
        menu_tools_of_->removeAction(act_tool_of_clean_); act_tool_of_clean_->disconnect();

        QAction *ma;
        ma = menu_results_->menuAction();
        ma->disconnect();
        mainMenu_->removeAction(ma);
        //menu_results_->deleteLater();

        ma = menu_parameters_->menuAction();
        ma->disconnect();
        mainMenu_->removeAction(ma);
        //menu_parameters_->deleteLater();

        ma = menu_actions_->menuAction();
        ma->disconnect();
        mainMenu_->removeAction(ma);
        //menu_actions_->deleteLater();

        ma = menu_tools_of_->menuAction();
        ma->disconnect();
        menu_tools_->removeAction(ma);

        ma = menu_tools_->menuAction();
        ma->disconnect();
        mainMenu_->removeAction(ma);
    }
    workbench::WidgetWithDynamicMenuEntries::removeMenu();
}




void AnalysisForm::closeEvent(QCloseEvent * event)
{
    if (is_modified_)
    {
      auto answer=QMessageBox::question(this, "Parameters unsaved",
                                        "The current parameters have been modified without saving.\n"
                                        "Do you wish to save them before closing?",
                                        QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
      if (answer==QMessageBox::Yes)
      {
        bool cancelled=false;
        saveParameters(&cancelled);
        if (cancelled) event->ignore();
      }
      else if (answer==QMessageBox::Cancel)
      {
        event->ignore();
      }
    }

    if (event->isAccepted())
    {
      QSettings settings("silentdynamics", "workbench");

      if (peditor_->hasVisualizer())
        settings.setValue("parameterEditor_wViz", peditor_->saveState());
      else
        settings.setValue("parameterEditor_woViz", peditor_->saveState());

      settings.setValue("pack_parameterset", QVariant(pack_parameterset_) );

      QMdiSubWindow::closeEvent(event);
    }

    if (event->isAccepted())
    {
      removeMenu();
    }

}




void AnalysisForm::onSaveParameters()
{
  saveParameters();
}




void AnalysisForm::saveParameters(bool *cancelled)
{
  if (ist_file_.empty())
  {
    saveParametersAs(cancelled);
  }
  else
  {
    insight::ParameterSet p = parameters();

    if (pack_parameterset_)
    {
      p.packExternalFiles();
    }
    else
    {
      p.removePackedData();
    }

    p.saveToFile(ist_file_, analysisName_);

    is_modified_=false;
    updateWindowTitle();
  }
}




void AnalysisForm::onSaveParametersAs()
{
  saveParametersAs();
}



void AnalysisForm::saveParametersAs(bool *cancelled)
{
//   emit apply();

  QFileDialog fd(this);
  fd.setOption(QFileDialog::DontUseNativeDialog, true);
  fd.setWindowTitle("Save Parameters");
  QStringList filters;
  filters << "Insight parameter sets (*.ist)";
  fd.setNameFilters(filters);

  QCheckBox* cb = new QCheckBox;
  cb->setText("Pack: embed externally referenced files into parameterset");
  QGridLayout *fdl = static_cast<QGridLayout*>(fd.layout());
  int last_row=fdl->rowCount(); // id of new row below
  fdl->addWidget(cb, last_row, 0, 1, -1);

  cb->setChecked(pack_parameterset_);

  if (fd.exec() == QDialog::Accepted)
  {
    QString fn = fd.selectedFiles()[0];
    pack_parameterset_ = cb->isChecked();
    updateSaveMenuLabel();

//     parameters_.saveToFile(fn.toStdString(), analysis_->type());
    ist_file_=fn.toStdString();

    if (!localCaseDirectory_)
    {
      resetLocalCaseDirectory(ist_file_.parent_path());
    }

    saveParameters(cancelled);

    if (cancelled) *cancelled=false;
  }
  else
  {
    if (cancelled) *cancelled=true;
  }
}



void AnalysisForm::resetLocalCaseDirectory(const boost::filesystem::path& lcd)
{
  localCaseDirectory_.reset(); // delete old one FIRST
  localCaseDirectory_.reset(
        new QCaseDirectoryState(this, lcd, true) );
  try
  {
    std::unique_ptr<insight::RemoteLocation> rl(
          new insight::RemoteLocation( lcd )
          );

    remoteExecutionConfiguration_.reset(
          new QRemoteExecutionState(this, lcd, *rl) );
  }
  catch (...)
  {
  }
}

void AnalysisForm::loadParameters(const boost::filesystem::path& fp)
{
  ist_file_=boost::filesystem::absolute(fp);

  if (!localCaseDirectory_)
  {
    resetLocalCaseDirectory(ist_file_.parent_path());
  }

  insight::ParameterSet ps = parameters();
  ps.readFromFile(ist_file_);
  peditor_->model()->resetParameters(
        ps,
        insight::Analysis::defaultParameters(analysisName_) );
}




void AnalysisForm::onLoadParameters()
{
  if (is_modified_)
  {
    auto answer = QMessageBox::question(this, "Parameters unsaved", "The current parameter set is unsaved and will be overwritten.\n"
                                                  "Do you wish to save them before continue?",
                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
    if (answer == QMessageBox::Yes)
    {
      onSaveParameters();
    }
    else if (answer == QMessageBox::Cancel)
    {
      return;
    }
  }

  QString fn = QFileDialog::getOpenFileName(this, "Open Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty())
  {
    loadParameters(fn.toStdString());
  }
}






void AnalysisForm::onShowParameterXML()
{
    QDialog *widget = new QDialog(this);
    Ui::XML_Display ui;
    ui.setupUi(widget);

//    Q_EMIT apply(); // apply all changes into parameter set

    boost::filesystem::path refPath = boost::filesystem::current_path();
    if (!ist_file_.empty())
    {
      refPath=ist_file_.parent_path();
    }
    std::ostringstream os;
    parameters().saveToStream(os, refPath, analysisName_);
    ui.textDisplay->setText(QString::fromStdString(os.str()));

    widget->exec();
}




void AnalysisForm::onConfigModification()
{
  is_modified_=true;
}





void AnalysisForm::onCreateReport()
{
  if (!results_.get())
  {
    QMessageBox::critical(this, "Error", "No results present!");
    return;
  }
  
  QString fn = QFileDialog::getSaveFileName
  (
      this, 
    "Save Report",
    QString::fromStdString(ist_file_.parent_path().string()),
    "PDF file (*.pdf);;LaTeX file (*.tex);;InsightCAE result set (*.isr)"
  );

  if (!fn.isEmpty())
  {
    boost::filesystem::path outpath=fn.toStdString();
    std::string ext=outpath.extension().string();

    if (ext.empty())
    {
      QMessageBox::critical(
            this,
            "Error!",
            "Please specify the file name with an extension!"
            );
      return;
    }

    if (boost::algorithm::to_lower_copy(ext)==".tex")
      {
        results_->writeLatexFile( outpath );
      }
    else if (boost::algorithm::to_lower_copy(ext)==".pdf")
      {
        results_->generatePDF( outpath );
      }
    else if (boost::algorithm::to_lower_copy(ext)==".isr")
      {
        results_->saveToFile ( outpath );
      }
    else
      {
        QMessageBox::critical(
              this,
              "Error!",
              "Unknown file format: "+fn
              );
        return;
      }

    QMessageBox::information(this, "Done!", "The report has been created as\n"+QString::fromStdString(outpath.string()) );
  }
}


