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

#include "base/parameterset.h"
#include "cadparametersetvisualizer.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
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
#include "base/translations.h"
#include "base/rapidxml.h"
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
#include "qtextensions.h"
#include "parametereditorwidget.h"

#include "iqcadmodel3dviewer/iqvtkcadmodel3dviewersettingsdialog.h"

namespace fs = boost::filesystem;






void AnalysisForm::saveState(
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &rootNode,
    const boost::filesystem::path &parentPath ) const
{
    {
        // current parameter set
        auto p = parameters()
                     .cloneAs<insight::ParameterSet>();

        if (pack_parameterset_)
        {
            p->pack();
        }
        else
        {
            p->clearPackedData();
        }

        insight::hierarchicalData::Element::OutputProperties op;
        op.skipParameterDescription=true;
        p->saveToNode(doc, rootNode, op);
    }

    {
        auto vs = insight::appendNode(doc, rootNode, "viewerState");
        peditor_->viewer()->saveState(doc, vs, parentPath);
    }
}


void AnalysisForm::restoreState(
    const rapidxml::xml_node<>& rootNode,
    const boost::filesystem::path& parentPath )
{
    auto ps = parameters().cloneAs<insight::ParameterSet>();
    ps->readFromRootNode(rootNode);
    ps->resolveRelativePaths(parentPath);
    psmodel_->resetParameterValues( *ps );

    if (auto *vs = rootNode.first_node("viewerState"))
    {
        peditor_->viewer()->restoreState(*vs, parentPath);
    }
}

void AnalysisForm::scheduleAutosave() const
{
    insight::dbg(insight::DetailedBusiness)<<"schedule autosave "
        <<autosaveLastWriteTimer_.isActive()<<" "<<autosaveLastWriteTimer_.remainingTime()
        <<" "
        <<autosaveMinIntervalTimer_.isActive()<<" "<<autosaveMinIntervalTimer_.remainingTime()
        <<std::endl;

    if ( autosaveLastWriteTimer_.isActive() )
    {
        // last write was recently, wait a little before write
        autosaveMinIntervalTimer_.stop();
    }

    if (!autosaveMinIntervalTimer_.isActive())
    {
        autosaveMinIntervalTimer_.start();
    }
}



AnalysisForm::AnalysisForm(
    QWidget* parent,
    const std::string& analysisName,
    bool logToConsole
    )
: QWidget(parent),
  IQExecutionWorkspace(this),
  isOpenFOAMAnalysis_(false),
  pack_parameterset_(true)
{
    autosaveLastWriteTimer_.setSingleShot(true);
    autosaveLastWriteTimer_.setInterval(autosaveMaxWaitInterval);
    autosaveMinIntervalTimer_.setSingleShot(true);
    autosaveMinIntervalTimer_.setInterval(autosaveMinInterval);
    autosaveMinIntervalTimer_.callOnTimeout(
        [this]()
        {
            autosaveLastWriteTimer_.start();
            autosave();
        });

    insight::CurrentExceptionContext ex(
        _("creating analysis form for analysis %s"), analysisName.c_str());

    setAttribute(Qt::WA_DeleteOnClose, true);

    // load default parameters
    auto defaultParams =
        std::make_unique<
            insight::ParameterSetWithGUIContext<
                insight::AnalysisParameterSet> >(
                    analysisName );

    isOpenFOAMAnalysis_ =
        defaultParams->hasPath("run/OFEname");


    ui = new Ui::AnalysisForm;
    QWidget* iw=this; //new QWidget(this);
    {
      insight::CurrentExceptionContext ex(_("setup user interface"));
        ui->setupUi(this);
        // ui->setupUi(iw);
        // setWidget(iw);
    }

    if (isOpenFOAMAnalysis_)
    {
        insight::CurrentExceptionContext ex(_("enable OpenFOAM controls"));

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

    QSplitter* spl=new QSplitter(Qt::Vertical);
    QWidget* lower = new QWidget;
    QHBoxLayout* hbl = new QHBoxLayout(lower);
    spl->addWidget(graphProgress_);
    spl->addWidget(lower);
    spl->setSizes( {500, 500} );

    log_=new LogViewerWidget(spl);
    hbl->addWidget(log_);

    QVBoxLayout* vbl=new QVBoxLayout;
    hbl->addLayout(vbl);
    save_log_btn_=new QPushButton(_("Save..."));
    send_log_btn_=new QPushButton(_("Email..."));
    clear_log_btn_=new QPushButton(_("Clear"));
    auto_scroll_down_btn_=new QPushButton(_("Auto Scroll"));
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
        insight::CurrentExceptionContext ex(_("redirect console"));

      cout_log_ = new IQDebugStream(std::cout);
      connect(cout_log_, &IQDebugStream::appendText,
              log_, &LogViewerWidget::appendDimmedLine);
      cerr_log_ = new IQDebugStream(std::cerr);
      connect(cerr_log_, &IQDebugStream::appendText,
              log_, &LogViewerWidget::appendErrorLine);
    }

    connect(ui->btnRun, &QPushButton::clicked, this, &AnalysisForm::onRunAnalysis);
    connect(ui->btnKill, &QPushButton::clicked, this, &AnalysisForm::onKillAnalysis);


    insight::CADParameterSetModelVisualizer::VisualizerFunctions::Function vizb =
        []( QObject*, IQParameterSetModel *, const boost::filesystem::path&, insight::ProgressDisplayer&)
        { return nullptr; };

    insight::ParameterSet_ValidatorPtr vali;

    auto& atab = insight::CADParameterSetModelVisualizer::visualizerForAnalysis();

    if (atab.count(analysisName))
    {
        insight::CurrentExceptionContext ex(_("create parameter set visualizer"));
        vizb = atab.lookup(analysisName);
    }

    if (insight::Analysis::validators().count(analysisName))
    {
        vali = insight::Analysis::validators()(analysisName);
    }

    auto vsplit = new QSplitter;
    vsplit->setOrientation(Qt::Vertical);
    ui->inputTabLayout->addWidget(vsplit);

    {
        insight::CurrentExceptionContext ex(_("create parameter set editor"));
        psmodel_=new IQParameterSetModel(std::move(defaultParams));
        peditor_=new ParameterEditorWidget(
            ui->inputTab,
            [this,vizb](QObject* _1,
               IQParameterSetModel *_2)
            {
                return vizb(
                        _1, _2,
                        localCaseDirectory(),
                        progressDisplayer_ );
            },
            [](
                 const std::string&,
                 QObject *,
                 IQCADModel3DViewer *,
                 IQParameterSetModel *
                 ) -> insight::GUIActionList { return {}; },
            vali
        );
        peditor_->setModel(psmodel_);
        connect(
              peditor_, &ParameterEditorWidget::updateSupplementedInputData,
              this, &AnalysisForm::onUpdateSupplementedInputData
              );

        //vsplit->addWidget(peditor_); // add later, depending on wizard or not
    }

    insight::CameraState cs;
    if (insight::CADParameterSetModelVisualizer
        ::defaultCameraStateForAnalysis().count(analysisName))
    {
        cs=insight::CADParameterSetModelVisualizer
            ::defaultCameraStateForAnalysis()(analysisName);
        peditor_->viewer()->setCameraState(cs);
    }



    connect(peditor_, &ParameterEditorWidget::parameterSetChanged, peditor_,
            [this]() {
                DBG_SLOT(ParameterEditorWidget::parameterSetChanged);

                modificationMade();
            } );

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

    if (insight::CADParameterSetModelVisualizer::createGUIWizardForAnalysis().count(
            analysisName ))
    {
#warning check StaticFunctionTable parameter: r-value ref?
        auto ppm=psmodel_;
        auto wiz=insight::CADParameterSetModelVisualizer::createGUIWizardForAnalysis()(
            analysisName, std::move(ppm)
            );

        auto *container=new QSplitter;
        container->setOrientation(Qt::Horizontal);
        container->addWidget(wiz);
        container->addWidget(peditor_);

        vsplit->addWidget(container);
    }
    else
    {
        vsplit->addWidget(peditor_);
    }


    sidtab_ = new QTableView;
    sidtab_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    sidtab_->setAlternatingRowColors(true);
    sidtab_->setModel(&supplementedInputDataModel_);
    vsplit->addWidget(sidtab_);
    sidtab_->hide();


    {
      insight::CurrentExceptionContext ex(_("create result viewer"));
        resultsViewer_=new IQResultSetDisplayerWidget(ui->outputTab);
        ui->outputTab->layout()->addWidget(resultsViewer_);
    }

    fileNameChanged.connect(
        [this](const boost::filesystem::path& newfn)
        {
            DBG_SLOT(fileNameChanged);

            auto ist_file=boost::filesystem::absolute(newfn);
            resetExecutionEnvironment(ist_file.parent_path());
            updateWindowTitle();

            // retrigger visualization build, since working directory is stored in supplementInputData
            sid_.reset();
            peditor_->rebuildVisualization();
        }
    );

    modificationStateChanged.connect(
        [this]()
        {
            DBG_SLOT(modificationStateChanged);

            updateWindowTitle();
        }
    );

    peditor_->viewer()->modificationMade.connect(
        this->modificationMade );

    insight::dbg()<<"update title"<<std::endl;
    updateWindowTitle();
}



const insight::ParameterSet& AnalysisForm::parameters() const
{
    return psmodel_->getParameterSet();
}



AnalysisForm::~AnalysisForm()
{
  prepareDeletion();
  currentWorkbenchAction_.reset();
  delete ui;
}




WidgetWithDynamicMenuEntries* AnalysisForm::createMenus(WorkbenchMainWindow* mw)
{
  insight::CurrentExceptionContext ex(_("create menus"));

    auto *dm = new WidgetWithDynamicMenuEntries(this);

    auto menu_parameters = dm->add(mw->menuBar()->addMenu(_("&Parameters")));
    auto menu_actions = dm->add(mw->menuBar()->addMenu(_("&Actions")));
    auto menu_results = dm->add(mw->menuBar()->addMenu(_("&Results")));
    auto menu_tools = dm->add(mw->menuBar()->addMenu(_("&Tools")));

    auto menu_tools_of = menu_tools->addMenu("&OpenFOAM");

    {
        auto cfgview=new QAction(_("3D &viewer settings..."), this);
        mw->settingsMenu_->addAction(cfgview);
        auto viewer_settings=dm->add(cfgview);
        connect( cfgview, &QAction::triggered, cfgview,
                [this]()
                {
                    DBG_SLOT(QAction::triggered);

                    IQVTKCADModel3DViewerSettingsDialog dlg(peditor_->viewer(), this);
                    dlg.exec();
                } );
    }


    auto act_undo=new QAction(_("&Undo parameter change"), this);
    menu_parameters->addAction( act_undo );
    psmodel_->addUndoAction(act_undo);

    auto act_redo=new QAction(_("&Redo parameter change"), this);
    menu_parameters->addAction( act_redo );
    psmodel_->addRedoAction(act_redo);


    act_save_=new QAction("&S", this);
    act_save_->setShortcut(Qt::CTRL + Qt::Key_S);
    menu_parameters->addAction( act_save_ );
    connect( act_save_, &QAction::triggered,
             this, &AnalysisForm::onSaveParameters );

    auto act_save_as_=new QAction(_("&Save parameter set as..."), this);
    menu_parameters->addAction( act_save_as_ );
    connect( act_save_as_, &QAction::triggered,
             this, &AnalysisForm::onSaveParametersAs );

    act_pack_=new QAction(_("&Pack external files into parameter file"), this);
    act_pack_->setCheckable(true);

    menu_parameters->addAction( act_pack_ );
    connect( act_pack_, &QAction::triggered, act_pack_,
             [&]()
             {
                DBG_SLOT(QAction::triggered);
               pack_parameterset_ = act_pack_->isChecked();
               updateSaveMenuLabel();
             }
    );

    updateSaveMenuLabel();

    auto act_merge_=new QAction(_("&Merge other parameter set into current..."), this);
    menu_parameters->addAction( act_merge_ );
    connect( act_merge_, &QAction::triggered, this, &AnalysisForm::onLoadParameters );

    auto act_param_show_=new QAction(_("Show in &XML format..."), this);
    menu_parameters->addAction( act_param_show_ );
    connect( act_param_show_, &QAction::triggered, this, &AnalysisForm::onShowParameterXML );



    auto act_run_=new QAction(_("&Run Analysis"), this);
    menu_actions->addAction( act_run_ );
    connect( act_run_, &QAction::triggered, this, &AnalysisForm::onRunAnalysis );
    auto act_kill_=new QAction(_("&Stop Analysis"), this);
    menu_actions->addAction( act_kill_ );
    connect( act_kill_, &QAction::triggered, this, &AnalysisForm::onKillAnalysis );


    {
        auto act=new QAction(_("&Load results..."), this);
        menu_results->addAction( act );
        connect( act, &QAction::triggered, resultsViewer_,
                 [this]() {
                    DBG_SLOT(QAction::triggere);

                ui->tabWidget->setCurrentWidget(ui->outputTab);
                    this->resultsViewer_->loadResultSet();
        } );
    }

    menu_results->addSeparator();

    auto act_save_res=new QAction(_("&Save results as..."), this);
    menu_results->addAction( act_save_res );
    connect( act_save_res, &QAction::triggered,
             resultsViewer_, &IQResultSetDisplayerWidget::saveResultSetAs );

    auto act_save_rpt_=new QAction(_("Create &report..."), this);
    menu_results->addAction( act_save_rpt_ );
    connect( act_save_rpt_, &QAction::triggered,
             resultsViewer_, &IQResultSetDisplayerWidget::renderReport );


    menu_results->addSeparator();

    {
        auto act=new QAction(_("Load &filter..."), this);
        menu_results->addAction( act );
        connect( act, &QAction::triggered,
                 resultsViewer_, &IQResultSetDisplayerWidget::loadFilter );
    }
    {
        auto act=new QAction(_("Sa&ve filter..."), this);
        menu_results->addAction( act );
        connect( act, &QAction::triggered,
                 resultsViewer_, &IQResultSetDisplayerWidget::saveFilter );
    }

    auto act_tool_of_paraview_=new QAction(_("Start ParaView in execution directory"), this);
    menu_tools_of->addAction( act_tool_of_paraview_ );
    connect( act_tool_of_paraview_, &QAction::triggered, this, &AnalysisForm::onStartPV );
    auto act_tool_of_clean_=new QAction(_("Clean OpenFOAM case..."), this);
    menu_tools_of->addAction( act_tool_of_clean_ );
    connect( act_tool_of_clean_, &QAction::triggered, this, &AnalysisForm::onCleanOFC );

    return dm;
}






void AnalysisForm::closeEvent(QCloseEvent * event)
{
    if (isModified())
    {

      auto answer=QMessageBox::question(
            this, _("Parameters unsaved"),
                  _("The current parameters have been modified without saving.\n"
                    "Do you wish to save them before closing?"),
                  QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel );

      if (answer==QMessageBox::Yes)
      {
        onSaveParameters();

        if (isModified())
        {
            // still modified => user must have cancelled
            event->ignore();
        }
      }
      else if (answer==QMessageBox::Cancel)
      {
        event->ignore();
      }
    }

    if (event->isAccepted())
    {

      if (peditor_->hasViewer())
        peditor_->viewer()->closeEvent(event);

      if (event->isAccepted())
      {
          QSettings settings("silentdynamics", "workbench");

          if (peditor_->hasVisualizer())
            settings.setValue("parameterEditor_wViz", peditor_->saveState());
          else
            settings.setValue("parameterEditor_woViz", peditor_->saveState());

          settings.setValue("pack_parameterset", QVariant(pack_parameterset_) );

          doCleanupOnRegularExit();

          QWidget::closeEvent(event);
      }
    }

}




void AnalysisForm::onSaveParameters()
{
    if (currentFileNameIsSet())
    {
        save();
    }
    else
    {
        onSaveParametersAs();
    }
}





void AnalysisForm::onSaveParametersAs()
{

  if (auto fn = getFileName(
          this, _("Save input parameters"),
          GetFileMode::Save,
          {
              {"ist", "Insight parameter sets", true},
              {"*", "any file", false}
          },
          boost::none,
          [this](QGridLayout *fdl)
          {
              auto *cb = new QCheckBox;
              cb->setText("Pack: embed externally referenced files into parameterset");
              int last_row=fdl->rowCount(); // id of new row below
              fdl->addWidget(cb, last_row, 0, 1, -1);

              cb->setChecked(pack_parameterset_);

              QObject::connect(cb, &QCheckBox::destroyed, cb,
                               [this,cb]()
                               {
                  DBG_SLOT(QCheckBox::destroyed);

                  pack_parameterset_=cb->isChecked();
              } );
          }
          ))
  {
    updateSaveMenuLabel();

    auto fp = fn.asFilesystemPath();

    psmodel_->resolveRelativePaths(
        fp.parent_path() );

    saveAs(fp);
  }
}




void AnalysisForm::onLoadParameters()
{
    loadParameters();
}




void AnalysisForm::loadParameters(
        boost::optional<boost::filesystem::path> fn)
{
  if (isModified())
  {
    auto answer = QMessageBox::question(
        this, _("Parameters unsaved"),
                _("The current parameter set is unsaved and will be overwritten.\n"
          "Do you wish to save them before continue?"),
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

  if (!fn)
  {
      if (auto fnd = getFileName(
        this, _("Open Parameters"),
        GetFileMode::Open,
        {{ "ist", _("Insight parameter sets (*.ist)") }} ) )
      {
          fn = fnd;
      }
  }

  if (fn)
  {
      auto asfn = AutosavableEditor
          ::autosaveFileName(*fn);

      auto fnToLoad= *fn;
      bool removeFromFile=false;

      using namespace boost::filesystem;
      if (exists(asfn)
          && (last_write_time(asfn) > last_write_time(*fn)))
      {
          auto answer = QMessageBox::question(
              this, _("Autosaved Parameters found"),
              _("There is an automatically saved version of the selected file available.\n"
                "Do you wish to load this instead of the selected file?"),
              QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
          if (answer == QMessageBox::Yes)
          {
              fnToLoad=asfn;
              removeFromFile=true;
          }
          else if (answer == QMessageBox::Cancel)
          {
              return;
          }
      }

      {
          std::shared_ptr<IQHierarchicalDataModel::UndoRecordingBlocker> blocker;
          if (auto *m = dynamic_cast<IQHierarchicalDataModel*>(peditor_->model()))
          {
              blocker=m->blockUndoRecording();
          }
          load(fnToLoad, *fn, removeFromFile);
      }
  }
}







void AnalysisForm::onShowParameterXML()
{
    QDialog *widget = new QDialog(this);
    Ui::XML_Display ui;
    ui.setupUi(widget);

//    Q_EMIT apply(); // apply all changes into parameter set

    std::ostringstream os;
    parameters().saveToStream(
        os,
        insight::hierarchicalData::Element::OutputProperties() );
    ui.textDisplay->setText(QString::fromStdString(os.str()));

    widget->exec();
}






void AnalysisForm::onUpdateSupplementedInputData(
    insight::supplementedInputDataBasePtr sid)
{
    DBG_SLOT(ParameterEditorWidget::updateSupplementedInputData);

  sid_=sid;
  supplementedInputDataModel_.reset( sid_->reportedSupplementQuantities() );
  if (sid_->reportedSupplementQuantities().size())
    sidtab_->show();
  else
      sidtab_->hide();
}







