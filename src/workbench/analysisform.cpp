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

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#endif

#include "base/analysis.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/remoteserverlist.h"

#include "analysisform.h"
#include "ui_analysisform.h"
#include "parameterwrapper.h"
#include "resultelementwrapper.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QScrollBar>
#include <QStatusBar>
#include "email.h"
#include "remotedirselector.h"

#include "ui_xml_display.h"

#include <cstdlib>
#include <memory>

#include "of_clean_case.h"

#include "remotesync.h"

int metaid1=qRegisterMetaType<insight::ParameterSet>("insight::ParameterSet");
int metaid2=qRegisterMetaType<insight::ResultSetPtr>("insight::ResultSetPtr");
int metaid3=qRegisterMetaType<insight::Exception>("insight::Exception");

namespace bf = boost::filesystem;


AnalysisWorker::AnalysisWorker(const std::shared_ptr<insight::Analysis>& analysis)
: analysis_(analysis)
{}

void AnalysisWorker::doWork(insight::ProgressDisplayer* pd)
{
  try
  {
    insight::ResultSetPtr results = (*analysis_)(pd);
    emit resultReady( results );
  }
  catch (const std::exception& e)
  {
    if ( const auto* ie = dynamic_cast<const insight::Exception*>(&e) )
    {
      emit error( insight::Exception(*ie) );
    }
    else
    {
      emit error( insight::Exception(e.what()) );
    }
  }
  catch (boost::thread_interrupted i)
  {
    emit killed();
  }
  catch (...)
  {
    emit error(insight::Exception("An unhandled exception occurred in the analysis thread!"));
  }

}




AnalysisForm::AnalysisForm(QWidget* parent, const std::string& analysisName)
: QMdiSubWindow(parent),
  analysisName_(analysisName),
  pack_parameterset_(true)
{
    // load default parameters
    auto defaultParams = insight::Analysis::defaultParameters(analysisName_);
    parameters_ = defaultParams;

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
      ui->paraviewbtn->setEnabled(true);
      connect( ui->paraviewbtn, &QPushButton::clicked, this, &AnalysisForm::onStartPV );
      ui->cleanbtn->setEnabled(true);
      connect( ui->cleanbtn, &QPushButton::clicked, this, &AnalysisForm::onCleanOFC );
      ui->wnow->setEnabled(true);
      connect( ui->wnow, &QPushButton::clicked, this, &AnalysisForm::onWnow );
      ui->wnowandstop->setEnabled(true);
      connect( ui->wnowandstop, &QPushButton::clicked, this, &AnalysisForm::onWnowAndStop );
    }

    QSplitter* spl=new QSplitter(Qt::Vertical);
    QWidget* lower = new QWidget;
    QHBoxLayout* hbl = new QHBoxLayout(lower);
    progdisp_=new GraphProgressDisplayer(spl);
    log_=new LogViewerWidget(spl);
    spl->addWidget(progdisp_);
    spl->addWidget(lower); //log_);
    hbl->addWidget(log_);
    QVBoxLayout* vbl=new QVBoxLayout;
    hbl->addLayout(vbl);
    save_log_btn_=new QPushButton("Save...");
    connect(save_log_btn_, &QPushButton::clicked, log_, &LogViewerWidget::saveLog);
    send_log_btn_=new QPushButton("Email...");
    connect(send_log_btn_, &QPushButton::clicked, log_, &LogViewerWidget::sendLog);
    clear_log_btn_=new QPushButton("Clear");
    connect(clear_log_btn_, &QPushButton::clicked, log_, &LogViewerWidget::clearLog);
    auto_scroll_down_btn_=new QPushButton("Auto Scroll");
    connect(auto_scroll_down_btn_, &QPushButton::clicked, log_, &LogViewerWidget::autoScrollLog);
    vbl->addWidget(save_log_btn_);
    vbl->addWidget(send_log_btn_);
    vbl->addWidget(clear_log_btn_);
    vbl->addWidget(auto_scroll_down_btn_);

    ui->runTabLayout->addWidget(spl);
    
    cout_log_ = new Q_DebugStream(std::cout);
    connect(cout_log_, &Q_DebugStream::appendText, log_, &LogViewerWidget::appendLine);
    cerr_log_ = new Q_DebugStream(std::cerr);
    connect(cerr_log_, &Q_DebugStream::appendText, log_, &LogViewerWidget::appendLine);

    this->setWindowTitle(analysisName_.c_str());
    connect(ui->runBtn, &QPushButton::clicked, this, &AnalysisForm::onRunAnalysis);
    connect(ui->killBtn, &QPushButton::clicked, this, &AnalysisForm::onKillAnalysis);


    insight::ParameterSet_VisualizerPtr viz;
    insight::ParameterSet_ValidatorPtr vali;

    try {
        viz = insight::Analysis::visualizer(analysisName_);
    } catch (const std::exception& e)
    {
      /* ignore, if non-existent */
      std::cout<<"Info: no visualizer for \""<<analysisName_<<"\" available."<<std::endl;
    }

    try {
        vali = insight::Analysis::validator(analysisName_);
    } catch (const std::exception& e)
    { /* ignore, if non-existent */ }

    peditor_=new ParameterEditorWidget(parameters_, defaultParams, ui->inputTab, viz, vali);
    ui->inputTabLayout->addWidget(peditor_);

    QObject::connect(this, &AnalysisForm::apply, peditor_, &ParameterEditorWidget::onApply);
    QObject::connect(this, &AnalysisForm::update, peditor_, &ParameterEditorWidget::onUpdate);

    connect(peditor_, &ParameterEditorWidget::parameterSetChanged,
            this, &AnalysisForm::onConfigModification);

    rtroot_=new QTreeWidgetItem(0);
    rtroot_->setText(0, "Results");
    ui->resultTree->setColumnCount(3);
    ui->resultTree->setHeaderLabels( QStringList() << "Result Element" << "Description" << "Current Value" );
    ui->resultTree->addTopLevelItem(rtroot_);

    QSettings settings("silentdynamics", "workbench");
    peditor_->restoreState(settings.value("parameterEditor").toByteArray());
    pack_parameterset_ = settings.value("pack_parameterset", QVariant(true)).toBool();


    connect(ui->btnSelectExecDir, &QPushButton::clicked,
            [&]()
            {
              QString dir = QFileDialog::getExistingDirectory(this, "Please select execution directory", ui->localDir->text());
              if (!dir.isEmpty())
              {
                ui->localDir->setText(dir);
              }
            }
    );

    for (const auto& i: insight::remoteServers)
    {
      ui->hostList->addItem( QString::fromStdString(i.first) );
    }

    connect(ui->btnSelectRemoteDir, &QPushButton::clicked,
            [&]()
    {
      RemoteDirSelector dlg(this, ui->hostList->currentText().toStdString() );
      if (dlg.exec() == QDialog::Accepted)
      {
          ui->hostList->setCurrentIndex( ui->hostList->findText( QString::fromStdString(dlg.selectedServer()) ) );
          ui->remoteDir->setText( QString::fromStdString(dlg.selectedRemoteDir().string()) );

//          std::ofstream cfg("meta.foam");
//          cfg << server_ << ":" << remoteDir_.string();

//          updateGUI();
      }
    }
    );

    connect(ui->btnUpload, &QPushButton::clicked,
            this, &AnalysisForm::onUpload);
    connect(ui->btnDownload, &QPushButton::clicked,
            this, &AnalysisForm::onDownload);

    progressbar_=new QProgressBar(this);
    auto *sb = new QStatusBar(this);
    this->layout()->addWidget(sb);
    sb->addPermanentWidget(progressbar_);
    connect(this, &AnalysisForm::statusMessage,
            sb, &QStatusBar::showMessage);
}

AnalysisForm::~AnalysisForm()
{
  if (workerThread_)
  {
    workerThread_->join();
  }

  delete ui;
}




void AnalysisForm::updateSaveMenuLabel()
{
  if (act_save_)
  {
    QString packed = pack_parameterset_ ? " (packed)" : "";
    act_save_->setText("&Save parameter set"+packed);
  }
  if (act_pack_)
  {
    act_pack_->setChecked(pack_parameterset_);
  }
}

bool AnalysisForm::hasValidExecutionPath() const
{
  return analysis_ || boost::filesystem::exists( ui->localDir->text().toStdString() );
}

boost::filesystem::path AnalysisForm::currentExecutionPath() const
{
  boost::filesystem::path exePath = ui->localDir->text().toStdString();
  if (analysis_) exePath = analysis_->executionPath();
  return exePath;
}

void AnalysisForm::onTogglePacking()
{
  pack_parameterset_ = act_pack_->isChecked();
  updateSaveMenuLabel();
}


void AnalysisForm::onRemoteServerChanged()
{
}

void AnalysisForm::onLockRemoteConfig()
{
  ui->hostList->setEnabled(false);
  ui->remoteDir->setEnabled(false);
  ui->btnSelectRemoteDir->setEnabled(false);
}

bool AnalysisForm::isReadyForRemoteRun() const
{
  std::string server = ui->hostList->currentText().toStdString();
  return ( (server != "localhost") && hasValidExecutionPath() );
}

bool AnalysisForm::isRemoteRunInitialized() const
{
  return ( isReadyForRemoteRun() && (!ui->hostList->isEnabled()) );
}


void AnalysisForm::onAutoSelectRemoteDir()
{
  if ( isReadyForRemoteRun() )
  {
    std::string server = ui->hostList->currentText().toStdString();


    auto i = insight::remoteServers.findServer(server);

    bf::path absloc=bf::canonical(bf::absolute(currentExecutionPath()));
    std::string casedirname = absloc.filename().string();

    bf::path remote_dir;
    {
      insight::MountRemote m(i.second.serverName_, i.second.defaultDir_);

      bf::path target_dir = boost::filesystem::unique_path( m.mountpoint()/("isofexecution-"+casedirname+"-%%%%%%%%") );

      remote_dir =
          i.second.defaultDir_ / boost::filesystem::make_relative(m.mountpoint(), target_dir);

//      boost::filesystem::create_directories(target_dir);
    }

    remotePaths_[server]=remote_dir;
    ui->remoteDir->setText( QString::fromStdString(remote_dir.string()) );
  }
}


void AnalysisForm::initializeRemoteRun()
{
  if ( isReadyForRemoteRun() )
  {
    if (ui->remoteDir->text().isEmpty())
      onAutoSelectRemoteDir();

    std::string server = ui->hostList->currentText().toStdString();
    bf::path remote_dir( ui->remoteDir->text().toStdString() );

    auto i = insight::remoteServers.findServer(server);

    {
      insight::MountRemote m(i.second.serverName_, i.second.defaultDir_);

      bf::path target_dir =
          m.mountpoint() / boost::filesystem::make_relative(i.second.defaultDir_, remote_dir);

      if (!bf::exists(target_dir))
        boost::filesystem::create_directories(target_dir);
    }

    onLockRemoteConfig();

    bf::path meta=currentExecutionPath()/"meta.foam";
    std::ofstream cfg(meta.c_str());
    cfg << i.second.serverName_ << ":" << remote_dir.string();
  }
}

void AnalysisForm::onUpload()
{
  if (!isRemoteRunInitialized())
  {
    initializeRemoteRun();

    insight::RemoteExecutionConfig rec(currentExecutionPath());

    auto *rstr = new insight::RunSyncToRemote(rec);

    connect(rstr, &insight::RunSyncToRemote::progressValueChanged,
            progressbar_, &QProgressBar::setValue);
    connect(rstr, &insight::RunSyncToRemote::progressTextChanged,
            this, [=](const QString& text) { emit statusMessage(text); } );
    connect(rstr, &insight::RunSyncToRemote::transferFinished,
            this, [&]()
    {
      progressbar_->setHidden(true);
      emit statusMessage("Transfer to remote location finished");
    });
    connect(rstr, &insight::RunSyncToRemote::transferFinished,
            rstr, &QObject::deleteLater);

    progressbar_->setHidden(false);
    emit statusMessage("Transfer to remote location started");

    rstr->start();
  }
}

void AnalysisForm::onDownload()
{
  if (isRemoteRunInitialized())
  {
    insight::RemoteExecutionConfig rec(currentExecutionPath());

    auto* rstl = new insight::RunSyncToLocal(rec);

    connect(rstl, &insight::RunSyncToLocal::progressValueChanged,
            progressbar_, &QProgressBar::setValue);
    connect(rstl, &insight::RunSyncToLocal::progressTextChanged,
            this, [=](const QString& text) { emit statusMessage(text); } );
    connect(rstl, &insight::RunSyncToLocal::transferFinished,
            rstl, &QObject::deleteLater);
    connect(rstl, &insight::RunSyncToLocal::transferFinished,
            this, [&]()
                  {
                    progressbar_->setHidden(true);
                    emit statusMessage("Transfer from remote location to local directory finished");
                  }
    );

    progressbar_->setHidden(false);
    emit statusMessage("Transfer from remote location to local directory started");

    rstl->start();
  }
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
             this, &AnalysisForm::onTogglePacking );

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
      settings.setValue("parameterEditor", peditor_->saveState());
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
    if (pack_parameterset_)
    {
      parameters_.packExternalFiles();
    }
    else
    {
      parameters_.removePackedData();
    }

    parameters_.saveToFile(ist_file_, analysisName_);
    is_modified_=false;
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

    saveParameters(cancelled);

    if (cancelled) *cancelled=false;
  }
  else
  {
    if (cancelled) *cancelled=true;
  }
}

void AnalysisForm::setExecutionPath(const boost::filesystem::path &p)
{
  ui->localDir->setText( p.c_str() );
}

void AnalysisForm::loadParameters(const boost::filesystem::path& fp)
{
  ist_file_=fp;
  parameters_.readFromFile(fp);
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
    emit update();
  }
}

void AnalysisForm::onShowParameterXML()
{
    QDialog *widget = new QDialog(this);
    Ui::XML_Display ui;
    ui.setupUi(widget);

    emit apply(); // apply all changes into parameter set

    std::ostringstream os;
    parameters_.saveToStream(os, currentExecutionPath(), analysisName_);
    ui.textDisplay->setText(QString::fromStdString(os.str()));

    widget->exec();
}

void AnalysisForm::onConfigModification()
{
  is_modified_=true;
}


void AnalysisForm::onRunAnalysis()
{
  if (isReadyForRemoteRun())
  {
    emit apply(); // apply all changes into parameter set

    // Remote execution
    std::string server( ui->hostList->currentText().toStdString() );
    bf::path localDir( currentExecutionPath() );

    // upload
    onUpload();

    if (isRemoteRunInitialized())
    {

      bf::path remoteDir(ui->remoteDir->text().toStdString()); // valid after initRemote!!

      bf::path jobfile_filename;
      if (!ist_file_.empty())
      {
        jobfile_filename = ist_file_.filename();
      }
      else
      {
        jobfile_filename = currentExecutionPath().replace_extension("ist");
      }

      insight::RemoteExecutionConfig rec(localDir);

      // create jobfile (temporary) and copy to execution dir
      bf::path tempinfile = bf::unique_path( localDir/"remote-input-%%%%-%%%%-%%%%-%%%%.ist" );

      parameters_.packExternalFiles();
      parameters_.saveToFile(tempinfile, analysisName_);
      if (!pack_parameterset_)
      {
        parameters_.removePackedData();
      }

#warning Separate thread required
      rec.putFile(tempinfile, jobfile_filename);

      // start job
      insight::TaskSpoolerInterface tsi( remoteDir/"tsp.socket", server );

      tsi.startJob({"analyze", "--workdir", remoteDir.string(), (remoteDir/jobfile_filename).string() });
    }
  }
  else
  {
    // local execution
    if (!workerThread_)
    {

        if (results_)
        {
            QMessageBox msgBox;
            msgBox.setText("There is currently a result set in memory!");
            msgBox.setInformativeText("If you continue, the results will be deleted and the execution directory on disk will be removed (only if it was created). Continue?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);


            if (msgBox.exec()==QMessageBox::Yes)
            {
                results_.reset();
            } else {
                return;
            }
        }


        boost::filesystem::path exePath( ui->localDir->text().toStdString() );
        // it's ok, if path is empty in the following checks

        if (isOpenFOAMAnalysis_)
        {
          bool evalOnly = insight::OpenFOAMAnalysis::Parameters(parameters_).run.evaluateonly;

          if (boost::filesystem::exists(exePath / "constant" / "polyMesh" ))
          {
            if (!evalOnly)
            {
              QMessageBox msgBox;
              msgBox.setText("There is already an OpenFOAM case present in the execution directory \""
                             +QString(exePath.c_str())+"\"!");
              msgBox.setInformativeText(
                    "Depending on the state of the data, the behaviour will be as follows:<br><ul>"
                    "<li>the mesh exists (\"constant/polyMesh/\") and a time directory exists (e.g. \"0/\"): the solver will be restarted,</li>"
                    "<li>only the mesh exists (\"constant/polyMesh/\"): mesh creation will be skipped but the dictionaries will be recreated</li>"
                    "</ul><br>If you are unsure about the validity of the case data, please consider to click on \"Cancel\" and clean the case directory first (click on clean button on the right).<br>"
                    "<br>"
                    "Continue?"
                    );
              msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
              msgBox.setDefaultButton(QMessageBox::Cancel);


              if (msgBox.exec()!=QMessageBox::Yes)
              {
                  return;
              }
            }
          }
          else
          {
            if (evalOnly)
            {
              QMessageBox msgBox;
              msgBox.setText("You have selected to run the evaluation only but there is no valid OpenFOAM case present in the execution directory \""
                             +QString(exePath.c_str())+"\"!");
              msgBox.setInformativeText(
                    "The subsequent step is likely to fail.<br>"
                    "Are you sure, that you want to continue?"
                    );
              msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
              msgBox.setDefaultButton(QMessageBox::Cancel);


              if (msgBox.exec()!=QMessageBox::Yes)
              {
                  return;
              }
            }
          }
        }

        emit apply(); // apply all changes into parameter set

        analysis_.reset( insight::Analysis::lookup(analysisName_, parameters_, exePath) );

        progdisp_->reset();

        ui->tabWidget->setCurrentWidget(ui->runTab);

        workerThread_.reset(new boost::thread( [&]() {
          AnalysisWorker worker(analysis_);

          connect(&worker, &AnalysisWorker::resultReady,
                  this, &AnalysisForm::onResultReady,
                  Qt::QueuedConnection);
          connect(&worker, &AnalysisWorker::error,
                  this, &AnalysisForm::onAnalysisErrorOccurred,
                  Qt::QueuedConnection);
          connect(&worker, &AnalysisWorker::killed,
                  this, &AnalysisForm::onAnalysisKilled,
                  Qt::QueuedConnection);

          worker.doWork(progdisp_);
        }
        ));



    }
  }
}


void AnalysisForm::onKillAnalysis()
{
  if (workerThread_)
  {
    workerThread_->interrupt();
  }
}

void AnalysisForm::onAnalysisKilled()
{
  if (workerThread_)
  {
    workerThread_->join();
    workerThread_.reset();

    QMessageBox::information(this, "Stopped!", "The analysis has been interrupted upon user request!");
  }
}

void AnalysisForm::onAnalysisErrorOccurred(insight::Exception e)
{
  if (workerThread_)
  {
    workerThread_->join();
    workerThread_.reset();
    throw e;
  }
}

void AnalysisForm::onResultReady(insight::ResultSetPtr results)
{
  if (workerThread_)
  {
    workerThread_->join();
    workerThread_.reset();

    results_=results;

  //   qDeleteAll(ui->outputContents->findChildren<ResultElementWrapper*>());
  //   addWrapperToWidget(*results_, ui->outputContents, this);

    rtroot_->takeChildren();
    addWrapperToWidget(*results_, rtroot_, this);
    ui->resultTree->doItemsLayout();
    ui->resultTree->expandAll();
    ui->resultTree->resizeColumnToContents(2);

    ui->tabWidget->setCurrentWidget(ui->outputTab);

    QMessageBox::information(this, "Finished!", "The analysis has finished");
  }
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
    QString(ist_file_.parent_path().c_str()),
    "PDF file (*.pdf);;LaTeX file (*.tex)"
  );

  if (!fn.isEmpty())
  {
    boost::filesystem::path outpath=fn.toStdString();
    std::string ext=outpath.extension().string();

    if (boost::algorithm::to_lower_copy(ext)==".tex")
      {
        results_->writeLatexFile( outpath );
      }
    else if (boost::algorithm::to_lower_copy(ext)==".pdf")
      {
        results_->generatePDF( outpath );
      }
    else
      {
        QMessageBox::critical(this, "Error!", "Unknown file format: "+fn);
        return;
      }

    QMessageBox::information(this, "Done!", QString("The report has been created as\n")+outpath.c_str());
  }
}

void AnalysisForm::onStartPV()
{
  if (hasValidExecutionPath())
  {
    emit apply(); // apply all changes into parameter set
    auto exePath = currentExecutionPath();
    ::system( boost::str( boost::format
          ("cd %s; isPV.py &" ) % exePath.string()
     ).c_str() );
  }
}

void AnalysisForm::onCleanOFC()
{
  if (hasValidExecutionPath())
  {
    const insight::OFEnvironment* ofc = nullptr;
    if (parameters_.contains("run/OFEname"))
    {
      std::string ofename=parameters_.getString("run/OFEname");
      ofc=&(insight::OFEs::get(ofename));
    }
    else
    {
      ofc=&(insight::OFEs::getCurrentOrPreferred());
    }

    OFCleanCaseDialog dlg(*ofc, currentExecutionPath(), this);
    dlg.exec();
  }
}


void AnalysisForm::onWnow()
{
  if (hasValidExecutionPath())
  {
    boost::filesystem::path exePath = currentExecutionPath();

    if (boost::filesystem::exists(exePath))
    {
      std::ofstream f( (exePath/"wnow").c_str() );
      f.close();
    }
  }
}


void AnalysisForm::onWnowAndStop()
{
  if (hasValidExecutionPath())
  {
    boost::filesystem::path exePath = currentExecutionPath();
    if (boost::filesystem::exists(exePath))
    {
      std::ofstream f( (exePath/"wnowandstop").c_str() );
      f.close();
    }
  }
}
