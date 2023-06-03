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
#include "iqresultsetmodel.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QScrollBar>
#include <QStatusBar>
#include <QSettings>
#include <QProcess>
#include <QCheckBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QMdiSubWindow>
#include <QHeaderView>
#include <QListView>

#include "email.h"

#include "ui_xml_display.h"

#include <cstdlib>
#include <memory>

#include "of_clean_case.h"
#include "remotesync.h"
#include "base/remoteserverlist.h"
#include "remotedirselector.h"
#include "base/wsllinuxserver.h"


#include "iqvtkcadmodel3dviewer.h"

namespace fs = boost::filesystem;



AnalysisForm::AnalysisForm(
    QWidget* parent,
    const std::string& analysisName,
    bool logToConsole
    )
: QMdiSubWindow(parent),
  IQExecutionWorkspace(this),
  analysisName_(analysisName),
  isOpenFOAMAnalysis_(false),
  pack_parameterset_(true),
  is_modified_(false)
{
    insight::CurrentExceptionContext ex("creating analysis form for analysis "+analysisName);

    setAttribute(Qt::WA_DeleteOnClose, true);

    // load default parameters
    auto defaultParams = insight::Analysis::defaultParameters(analysisName_);

    try
    {
      defaultParams.getString("run/OFEname"); // try to access
      isOpenFOAMAnalysis_ = true;
    }
    catch (...)
    {
      isOpenFOAMAnalysis_ = false;
    }

    ui = new Ui::AnalysisForm;
    QWidget* iw=new QWidget(this);
    {
        insight::CurrentExceptionContext ex("setup user interface");
        ui->setupUi(iw);
        setWidget(iw);
    }

    if (isOpenFOAMAnalysis_)
    {
        insight::CurrentExceptionContext ex("enable OpenFOAM controls");

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

    insight::dbg()<<"graphprogressdisplayer"<<std::endl;
    graphProgress_=new GraphProgressDisplayer;
    actionProgress_=new insight::QActionProgressDisplayerWidget;

    QSplitter* spl=new QSplitter(Qt::Vertical);
    QWidget* lower = new QWidget;
    QHBoxLayout* hbl = new QHBoxLayout(lower);
    spl->addWidget(graphProgress_);
    spl->addWidget(lower);
    spl->setSizes( {500, 500} );
    insight::dbg()<<"log viewer"<<std::endl;
    log_=new LogViewerWidget(spl);
    hbl->addWidget(log_);

    insight::dbg()<<"more buttons"<<std::endl;
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
    
    progressDisplayer_.setOp( insight::CombinedProgressDisplayer::OR );
    progressDisplayer_.add(graphProgress_);
    progressDisplayer_.add(actionProgress_);
    progressDisplayer_.add(log_);

    if (!logToConsole)
    {
        insight::CurrentExceptionContext ex("redirect console");

      cout_log_ = new IQDebugStream(std::cout);
      connect(cout_log_, &IQDebugStream::appendText,
              log_, &LogViewerWidget::appendDimmedLine);
      cerr_log_ = new IQDebugStream(std::cerr);
      connect(cerr_log_, &IQDebugStream::appendText,
              log_, &LogViewerWidget::appendErrorLine);
    }

    insight::dbg()<<"update title"<<std::endl;
    updateWindowTitle();
    connect(ui->btnRun, &QPushButton::clicked, this, &AnalysisForm::onRunAnalysis);
    connect(ui->btnKill, &QPushButton::clicked, this, &AnalysisForm::onKillAnalysis);


    insight::ParameterSetVisualizerPtr viz;
    insight::ParameterSet_ValidatorPtr vali;

    try
    {
        insight::CurrentExceptionContext ex("create parameter set visualizer");
        viz = insight::Analysis::visualizer(analysisName_);
        viz ->setProgressDisplayer(&progressDisplayer_);
    }
    catch (const std::exception& e)
    {
      /* ignore, if non-existent */
      std::cout<<"Info: no visualizer for \""<<analysisName_<<"\" available."<<std::endl;
    }

    try
    {
        vali = insight::Analysis::validator(analysisName_);
    }
    catch (const std::exception& e)
    { /* ignore, if non-existent */ }

    auto vsplit = new QSplitter;
    vsplit->setOrientation(Qt::Vertical);
    ui->inputTabLayout->addWidget(vsplit);

    {
        insight::CurrentExceptionContext ex("create parameter set editor");
        peditor_=new ParameterEditorWidget(/*parameters_*/defaultParams, defaultParams, ui->inputTab, viz, vali);
        connect(
              peditor_, &ParameterEditorWidget::updateSupplementedInputData,
              this, &AnalysisForm::onUpdateSupplementedInputData
              );
        //ui->inputTabLayout->addWidget(peditor_);
        vsplit->addWidget(peditor_);
    }
    peditor_->model()->setAnalysisName(analysisName_);

    sidtab_ = new QTableView;
    sidtab_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    sidtab_->setAlternatingRowColors(true);
    sidtab_->setModel(&supplementedInputDataModel_);
    //ui->inputTabLayout->addWidget(sidtab_);
    vsplit->addWidget(sidtab_);
    sidtab_->hide();

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


    IQExecutionWorkspace::initializeToDefaults();


    {
        insight::CurrentExceptionContext ex("create result viewer");
        resultsViewer_=new IQResultSetDisplayerWidget(ui->outputTab);
        ui->outputTab->layout()->addWidget(resultsViewer_);
    }

}



const insight::ParameterSet& AnalysisForm::parameters() const
{
  return peditor_->model()->getParameterSet();
}



AnalysisForm::~AnalysisForm()
{
  prepareDeletion();
  currentWorkbenchAction_.reset();
  delete ui;
}




WidgetWithDynamicMenuEntries* AnalysisForm::createMenus(QMenuBar* mainMenu)
{
    insight::CurrentExceptionContext ex("create menus");

    auto *dm = new WidgetWithDynamicMenuEntries(this);

    auto menu_parameters = dm->add(mainMenu->addMenu("&Parameters"));
    auto menu_actions = dm->add(mainMenu->addMenu("&Actions"));
    auto menu_results = dm->add(mainMenu->addMenu("&Results"));
    auto menu_tools = dm->add(mainMenu->addMenu("&Tools"));

    auto menu_tools_of = menu_tools->addMenu("&OpenFOAM");

    act_save_=new QAction("&S", this);
    act_save_->setShortcut(Qt::CTRL + Qt::Key_S);
    menu_parameters->addAction( act_save_ );
    connect( act_save_, &QAction::triggered,
             this, &AnalysisForm::onSaveParameters );

    auto act_save_as_=new QAction("&Save parameter set as...", this);
    menu_parameters->addAction( act_save_as_ );
    connect( act_save_as_, &QAction::triggered,
             this, &AnalysisForm::onSaveParametersAs );

    act_pack_=new QAction("&Pack external files into parameter file", this);
    act_pack_->setCheckable(true);

    menu_parameters->addAction( act_pack_ );
    connect( act_pack_, &QAction::triggered, act_pack_,
             [&]()
             {
               pack_parameterset_ = act_pack_->isChecked();
               updateSaveMenuLabel();
             }
    );

    updateSaveMenuLabel();

    auto act_merge_=new QAction("&Merge other parameter set into current...", this);
    menu_parameters->addAction( act_merge_ );
    connect( act_merge_, &QAction::triggered, this, &AnalysisForm::onLoadParameters );

    auto act_param_show_=new QAction("&Show in XML format", this);
    menu_parameters->addAction( act_param_show_ );
    connect( act_param_show_, &QAction::triggered, this, &AnalysisForm::onShowParameterXML );



    auto act_run_=new QAction("&Run Analysis", this);
    menu_actions->addAction( act_run_ );
    connect( act_run_, &QAction::triggered, this, &AnalysisForm::onRunAnalysis );
    auto act_kill_=new QAction("&Stop Analysis", this);
    menu_actions->addAction( act_kill_ );
    connect( act_kill_, &QAction::triggered, this, &AnalysisForm::onKillAnalysis );


    {
        auto act=new QAction("&Load results...", this);
        menu_results->addAction( act );
        connect( act, &QAction::triggered, resultsViewer_,
                 [this]() { this->resultsViewer_->loadResultSet(analysisName_); } );
    }

    menu_results->addSeparator();

    auto act_save_res=new QAction("&Save results as...", this);
    menu_results->addAction( act_save_res );
    connect( act_save_res, &QAction::triggered,
             resultsViewer_, &IQResultSetDisplayerWidget::saveResultSetAs );

    auto act_save_rpt_=new QAction("Create &report...", this);
    menu_results->addAction( act_save_rpt_ );
    connect( act_save_rpt_, &QAction::triggered,
             resultsViewer_, &IQResultSetDisplayerWidget::renderReport );


    menu_results->addSeparator();

    {
        auto act=new QAction("Load &filter...", this);
        menu_results->addAction( act );
        connect( act, &QAction::triggered,
                 resultsViewer_, &IQResultSetDisplayerWidget::loadFilter );
    }
    {
        auto act=new QAction("Sa&ve filter...", this);
        menu_results->addAction( act );
        connect( act, &QAction::triggered,
                 resultsViewer_, &IQResultSetDisplayerWidget::saveFilter );
    }

    auto act_tool_of_paraview_=new QAction("Start ParaView in execution directory", this);
    menu_tools_of->addAction( act_tool_of_paraview_ );
    connect( act_tool_of_paraview_, &QAction::triggered, this, &AnalysisForm::onStartPV );
    auto act_tool_of_clean_=new QAction("Clean OpenFOAM case...", this);
    menu_tools_of->addAction( act_tool_of_clean_ );
    connect( act_tool_of_clean_, &QAction::triggered, this, &AnalysisForm::onCleanOFC );

    return dm;
}






void AnalysisForm::closeEvent(QCloseEvent * event)
{
    if (is_modified_)
    {

      auto answer=QMessageBox::question(
                  this, "Parameters unsaved",
                  "The current parameters have been modified without saving.\n"
                  "Do you wish to save them before closing?",
                  QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel );

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
      peditor_->viewer()->closeEvent(event);

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

    ist_file_=fn.toStdString();

    if (!hasLocalWorkspace())
    {
      resetExecutionEnvironment(ist_file_.parent_path());
    }

    saveParameters(cancelled);

    if (cancelled) *cancelled=false;
  }
  else
  {
    if (cancelled) *cancelled=true;
  }
}





void AnalysisForm::loadParameters(const boost::filesystem::path& fp)
{
  ist_file_=boost::filesystem::absolute(fp);

  if (!hasLocalWorkspace())
  {
    resetExecutionEnvironment(ist_file_.parent_path());
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
    auto answer = QMessageBox::question(
                this, "Parameters unsaved",
                "The current parameter set is unsaved and will be overwritten.\n"
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

  QString fn = QFileDialog::getOpenFileName(
              this, "Open Parameters",
              QString(),
              "Insight parameter sets (*.ist)" );

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




void AnalysisForm::onUpdateSupplementedInputData(insight::supplementedInputDataBasePtr sid)
{
  supplementedInputDataModel_.reset( sid->reportedSupplementQuantities() );
  if (sid->reportedSupplementQuantities().size())
    sidtab_->show();
  else
      sidtab_->hide();
}







