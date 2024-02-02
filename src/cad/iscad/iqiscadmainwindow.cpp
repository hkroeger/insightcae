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
 */

#include "base/boost_include.h"

#include "iqiscadmainwindow.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqiscadmodelwindow.h"
#include "iqiscadmodelscriptedit.h"
#include "iqcaditemmodel.h"

#include "qmodeltree.h"
#include "qmodelstepitem.h"
#include "qvariableitem.h"
#include "qdatumitem.h"
#include "qevaluationitem.h"
#include "iqiscadsyntaxhighlighter.h"
#include "occtools.h"


#include <QSignalMapper>
#include <QStatusBar>
#include <QSettings>
#include <QAction>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QFileSystemModel>
#include <QProgressBar>

#include "iqdebugstream.h"

#include "cadfeatures/modelfeature.h"
#include "modelcomponentselectordlg.h"
#include "insertfeaturedlg.h"

#include "datum.h"

#include "base/qt5_helper.h"
#include "base/toolkitversion.h"
#include "base/translations.h"

 

void IQISCADMainWindow::connectMenuToModel(IQISCADModelWindow* me, IQISCADModelWindow* lme)
{
  for(auto a: act_) a.second->disconnect();

    if (lme!=NULL)
      {
        disconnect( lme->modelEdit(), &IQISCADModelScriptEdit::modelUpdated,
                    this, &IQISCADMainWindow::onUpdateClipPlaneMenu );
        disconnect( lme->modelEdit(), &IQISCADModelScriptEdit::openModel,
                    this, &IQISCADMainWindow::onLoadModelFile );
      }

    disconnect(bgparsestopbtn_, &QPushButton::clicked, 0, 0);

    if (me)
    {
        connect(act_[save], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::saveModel);
        connect(act_[saveas], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::saveModelAs);
        connect(act_[rebuild], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::rebuildModel);
        connect(act_[rebuild_UTC], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::rebuildModelUpToCursor);
        connect(act_[insert_section_comment], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::insertSectionCommentAtCursor);
        connect(act_[insert_feat], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::insertFeatureAtCursor);
        connect(act_[insert_import], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::insertImportedModelAtCursor);
        connect(act_[insert_loadmodel], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::insertLibraryModelAtCursor);
        connect(act_[insert_component_name], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::insertComponentNameAtCursor);
        connect(act_[clear_cache], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::clearCache);
        connect(act_[editor_font_larger], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::onIncreaseFontSize);
        connect(act_[editor_font_smaller], &QAction::triggered,
                me->modelEdit(), &IQISCADModelScriptEdit::onDecreaseFontSize);


        connect(act_[fit_all], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::fitAll);

        connect(act_[view_plusx], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::viewRight);
        connect(act_[view_minusx], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::viewLeft);
        connect(act_[view_plusy], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::viewBack);
        connect(act_[view_minusy], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::viewFront);
        connect(act_[view_plusz], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::viewTop);
        connect(act_[view_minusz], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::viewBottom);

//        connect(act_[toggle_grid], &QAction::triggered,
//                me->viewer(), &QoccViewWidget::toggleGrid);
        connect(act_[toggle_clipxy], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::toggleClipXY);
        connect(act_[toggle_clipyz], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::toggleClipYZ);
        connect(act_[toggle_clipxz], &QAction::triggered,
                me->viewer(), &IQCADModel3DViewer::toggleClipXZ);
//        connect(act_[background_color], &QAction::triggered,
//                me->viewer(), &QoccViewWidget::background);
//        connect(act_[display_all_shaded], &QAction::triggered,
//                me->modeltree(), &QModelTree::allShaded);
//        connect(act_[display_all_wire], &QAction::triggered,
//                me->modeltree(), &QModelTree::allWireframe);
//        connect(act_[reset_shading], &QAction::triggered,
//                me->modeltree(), &QModelTree::resetViz);

        connect(act_[measure_distance], &QAction::triggered,
                me->viewer(), &IQISCADModelWindow::Model3DViewer::onMeasureDistance);

        connect(act_[measure_diameter], &QAction::triggered,
                me->viewer(), &IQISCADModelWindow::Model3DViewer::onMeasureDiameter);

        connect(act_[sel_pts], &QAction::triggered,
                me->viewer(), &IQISCADModelWindow::Model3DViewer::onSelectPoints );

        connect(act_[sel_edgs], &QAction::triggered,
                me->viewer(), &IQISCADModelWindow::Model3DViewer::onSelectEdges );

        connect(act_[sel_faces], &QAction::triggered,
                me->viewer(), &IQISCADModelWindow::Model3DViewer::onSelectFaces );

        connect(act_[sel_solids], &QAction::triggered,
                me->viewer(), &IQISCADModelWindow::Model3DViewer::onSelectSolids );

        me->model()->populateClipPlaneMenu(clipplanemenu_, me->viewer());

        connect(bgparsestopbtn_, &QPushButton::clicked,
                me->modelEdit(), &IQISCADModelScriptEdit::onCancelRebuild);

        connect(me->modelEdit(), &IQISCADModelScriptEdit::modelUpdated,
                this, &IQISCADMainWindow::onUpdateClipPlaneMenu);
        
        connect(me->modelEdit(), &IQISCADModelScriptEdit::openModel,
                this, &IQISCADMainWindow::onLoadModelFile);

      }
}



void IQISCADMainWindow::loadModel()
{
    QString fn=QFileDialog::getOpenFileName(
          this, _("Select file"),
          "",
          _("ISCAD Model Files (*.iscad)"));
    if (fn!="")
    {
        insertModel(qPrintable(fn))->modelEdit()->unsetUnsavedState();
    }
}


void IQISCADMainWindow::activateModel(int tabindex)
{
    if ( (tabindex>=0) && (tabindex!=lastTabIndex_) )
    {
        IQISCADModelWindow* lme=NULL;
        if (lastTabIndex_>=0) lme=static_cast<IQISCADModelWindow*>(modelTabs_->widget(lastTabIndex_));

        connectMenuToModel(static_cast<IQISCADModelWindow*>(modelTabs_->widget(tabindex)), lme);
        lastTabIndex_=tabindex;
    }
}

// void ISCADMainWindow::deactivateModel(ISCADModel* model)
// {
// }


void IQISCADMainWindow::onUpdateTabTitle(IQISCADModelWindow* model, const boost::filesystem::path& filepath, bool isUnSaved)
{
    int i=modelTabs_->indexOf(model);
    if (i>=0)
    {
        modelTabs_->setTabText(i, QString(isUnSaved?"*":"") + QString::fromStdString(filepath.filename().string()));
        modelTabs_->setTabToolTip(i, QString::fromStdString(filepath.string()));
    }
}

void IQISCADMainWindow::onCloseModel(int tabindex)
{
    auto model = static_cast<IQISCADModelScriptEdit*>(modelTabs_->widget(tabindex));
    if (model)
    {
        QCloseEvent ev;
        model->closeEvent(&ev);
        if (ev.isAccepted())
            modelTabs_->removeTab(tabindex);
    }
}

void IQISCADMainWindow::onUpdateClipPlaneMenu(int errorState)
{
  if (errorState==0)
    {
      if (IQISCADModelWindow *me = static_cast<IQISCADModelWindow*>(modelTabs_->currentWidget()))
      {
          me->model()->populateClipPlaneMenu(clipplanemenu_, me->viewer());
      }
    }
}

void IQISCADMainWindow::onNewModel()
{
  insertEmptyModel();
}

void IQISCADMainWindow::onLoadModelFile(const boost::filesystem::path& modelfile)
{
    insertModel(modelfile);
}



IQISCADMainWindow::IQISCADMainWindow(QWidget* parent, bool nolog)
: QMainWindow(parent),
  lastTabIndex_(-1)
{
    
    setWindowIcon(QIcon(":/resources/logo_insight_cae.png"));

    QSplitter *spl0=new QSplitter(Qt::Vertical);
    QSplitter *spl=new QSplitter(Qt::Horizontal);
    setCentralWidget(spl0);
    spl0->addWidget(spl);
    log_=new QTextEdit;

    if (!nolog)
    {
      logger_=new IQDebugStream(std::cout); // ceases to work with multithreaded bg parsing
      connect(logger_, &IQDebugStream::appendText,
              log_, &QTextEdit::append);

    }
    
    spl0->addWidget(log_);
    
    const QString rootPath = QDir::currentPath();
    fileModel_ = new QFileSystemModel;
    fileTree_=new QTreeView;
    fileTree_->setModel( fileModel_ );

    fileTree_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QStringList filter;
    filter << "*.iscad";
    fileModel_->setNameFilters( filter );
    fileModel_->setNameFilterDisables(false);
    fileModel_->setRootPath( rootPath );
    fileTree_->setRootIndex( fileModel_->index( rootPath ) );
    fileTree_->expandAll();
    for (int i = 1; i < fileModel_->columnCount(); ++i)
        fileTree_->hideColumn(i);
    connect(fileTree_, &QTreeView::doubleClicked, this, &IQISCADMainWindow::onFileClicked);
    fileTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTree_, &QTreeView::customContextMenuRequested,
            this, &IQISCADMainWindow::onShowFileTreeContextMenu);
    spl->addWidget(fileTree_);
    
    modelTabs_=new QTabWidget;
    modelTabs_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    modelTabs_->setTabsClosable(true);
    spl->addWidget(modelTabs_);
    connect(modelTabs_, &QTabWidget::currentChanged, this, &IQISCADMainWindow::activateModel);
    connect(modelTabs_, &QTabWidget::tabCloseRequested, this, &IQISCADMainWindow::onCloseModel);

    spl->setStretchFactor(0,0);
    spl->setStretchFactor(1,1);

    spl0->setStretchFactor(0, 5);
    spl0->setStretchFactor(1, 0);

    progressbar_=new QProgressBar;
    bgparsestopbtn_=new QPushButton(_("STOP"));
    statusBar()->addPermanentWidget(progressbar_);
    statusBar()->addPermanentWidget(bgparsestopbtn_);

    QMenu *fmenu = menuBar()->addMenu(_("&File"));
    QMenu *mmenu = menuBar()->addMenu(_("&Model"));
    QMenu *vmenu = menuBar()->addMenu(_("&View"));
    QMenu *emenu = menuBar()->addMenu(_("&Editor"));
    QMenu *msmenu = menuBar()->addMenu(_("M&easure"));
    QMenu *selmenu = menuBar()->addMenu(_("&Selection"));
    QMenu *helpmenu = menuBar()->addMenu(_("&Help"));

    QAction* ab = new QAction("About...", this);
    helpmenu->addAction( ab );
    connect(ab, &QAction::triggered,
            [&]()
            {
              QMessageBox::information(
                    this,
                    _("ISCAD Information"),
                    QString(_("InsightCAE CAD Script Editor\n"
                      "Version %1\n")).arg(QString::fromStdString(insight::ToolkitVersion::current().toString()))
                    );
            }
    );

    QAction *act;

    act=new QAction(_("&New"), this);
    fmenu->addAction(act);
    connect(act, &QAction::triggered, this, &IQISCADMainWindow::onNewModel);

    act=new QAction(_("&Load"), this);
    fmenu->addAction(act);
    connect(act, &QAction::triggered, this, &IQISCADMainWindow::loadModel);

    act_[save] = new QAction(_("&Save"), this);
    act_[save]->setShortcut(Qt::ControlModifier + Qt::Key_S);
    fmenu->addAction(act_[save]);

    act_[saveas] = new QAction(_("&Save as..."), this);
    fmenu->addAction(act_[saveas]);

    fmenu->addSeparator();
    act =new QAction(_("&Quit"), this);
    act->setShortcut(Qt::AltModifier + Qt::Key_F4);
    fmenu->addAction(act);
    connect(act, &QAction::triggered, this, &IQISCADMainWindow::close);


    act_[rebuild] = new QAction(_("&Rebuild model"), this);
    act_[rebuild]->setShortcut(Qt::ControlModifier + Qt::Key_Return);
    mmenu->addAction(act_[rebuild]);

    act_[rebuild_UTC] = new QAction(_("&Rebuild model up to cursor"), this);
    act_[rebuild_UTC]->setShortcut(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_Return);
    mmenu->addAction(act_[rebuild_UTC]);

    mmenu->addSeparator();
    
    act_[insert_feat] = new QAction(_("Insert &feature..."), this);
    act_[insert_feat]->setShortcut(Qt::ControlModifier + Qt::Key_F);
    mmenu->addAction(act_[insert_feat]);

    act_[insert_import] = new QAction(_("Insert &imported STEP, IGES, BREP..."), this);
    act_[insert_import]->setShortcut(Qt::ControlModifier + Qt::Key_M);
    mmenu->addAction(act_[insert_import]);

    act_[insert_loadmodel] = new QAction(_("Insert &model from library..."), this);
    act_[insert_loadmodel]->setShortcut(Qt::ControlModifier + Qt::Key_L);
    mmenu->addAction(act_[insert_loadmodel]);

    act_[insert_component_name] = new QAction(_("Inspect model &symbols..."), this);
    act_[insert_component_name]->setShortcut(Qt::ControlModifier + Qt::Key_I);
    mmenu->addAction(act_[insert_component_name]);

    
    act_[insert_section_comment] = new QAction(_("Insert comment: new section..."), this);
    act_[insert_section_comment]->setShortcut(Qt::AltModifier + Qt::Key_S);
    mmenu->addAction(act_[insert_section_comment]);

    mmenu->addSeparator();

    act_[clear_cache] = new QAction(_("C&lear cache"), this);
    mmenu->addAction(act_[clear_cache]);

    act_[fit_all] = new QAction(_("Fit &all"), this);
    act_[fit_all]->setShortcut(Qt::ControlModifier + Qt::Key_A);
    vmenu->addAction(act_[fit_all]);

    QMenu* directionmenu=vmenu->addMenu(_("Standard views"));
    directionmenu->addAction( act_[view_plusx] = new QAction(("+X"), this) );
    act_[view_plusx]->setShortcut(Qt::ControlModifier + Qt::Key_X);
    directionmenu->addAction( act_[view_minusx] = new QAction(("-X"), this) );
    act_[view_minusx]->setShortcut(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_X);

    directionmenu->addAction( act_[view_plusy] = new QAction(("+Y"), this) );
    act_[view_plusy]->setShortcut(Qt::ControlModifier + Qt::Key_Y);
    directionmenu->addAction( act_[view_minusy] = new QAction(("-Y"), this) );
    act_[view_minusy]->setShortcut(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_Y);

    directionmenu->addAction( act_[view_plusz] = new QAction(("+Z"), this) );
    act_[view_plusz]->setShortcut(Qt::ControlModifier + Qt::Key_Z);
    directionmenu->addAction( act_[view_minusz] = new QAction(("-Z"), this) );
    act_[view_minusz]->setShortcut(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_Z);

    act_[toggle_grid] = new QAction(_("Toggle &grid"), this);
    vmenu->addAction(act_[toggle_grid]);
    
    act_[toggle_clipxy] = new QAction(_("Toggle clip plane (&XY)"), this);
    vmenu->addAction(act_[toggle_clipxy]);
    act_[toggle_clipyz] = new QAction(_("Toggle clip plane (&YZ)"), this);
    vmenu->addAction(act_[toggle_clipyz]);
    act_[toggle_clipxz] = new QAction(_("Toggle clip plane (X&Z)"), this);
    vmenu->addAction(act_[toggle_clipxz]);
    
    clipplanemenu_=vmenu->addMenu(_("Clip at datum plane"));
    clipplanemenu_->setDisabled(true);
    
    act_[background_color] = new QAction(_("Change background color..."), this);
    vmenu->addAction(act_[background_color]);
    act_[display_all_shaded] = new QAction(_("Display all &shaded"), this);
    vmenu->addAction(act_[display_all_shaded]);
    act_[display_all_wire] = new QAction(_("Display all &wireframe"), this);
    vmenu->addAction(act_[display_all_wire]);
    act_[reset_shading] = new QAction(_("&Reset shading and visibility"), this);
//     act->setShortcut(Qt::ControlModifier + Qt::Key_A);
    vmenu->addAction(act_[reset_shading]);


    act_[measure_distance]=new QAction(_("Distance between points"), this);
    act_[measure_distance]->setShortcut(Qt::ControlModifier + Qt::Key_D);
    msmenu->addAction(act_[measure_distance]);

    act_[measure_diameter]=new QAction(_("Curve diameter and center point"), this);
    msmenu->addAction(act_[measure_diameter]);

    act_[sel_pts]=new QAction(_("Select vertices"), this);
    selmenu->addAction(act_[sel_pts]);

    act_[sel_edgs]=new QAction(_("Select edges"), this);
    selmenu->addAction(act_[sel_edgs]);

    act_[sel_faces]=new QAction(_("Select faces"), this);
    selmenu->addAction(act_[sel_faces]);

    act_[sel_solids]=new QAction(_("Select solids"), this);
    selmenu->addAction(act_[sel_solids]);


    act_[editor_font_larger]=new QAction(_("&Increase font size"), this);
    act_[editor_font_larger]->setShortcut(Qt::ControlModifier + Qt::Key_Plus);
    emenu->addAction(act_[editor_font_larger]);

    act_[editor_font_smaller]=new QAction(_("&Decrease font size"), this);
    act_[editor_font_smaller]->setShortcut(Qt::ControlModifier + Qt::Key_Minus);
    emenu->addAction(act_[editor_font_smaller]);


    QSettings settings("silentdynamics", "iscad");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

 
}

IQISCADMainWindow::~IQISCADMainWindow()
{
}




void IQISCADMainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    
    for (int i=0; i<modelTabs_->count(); i++)
    {
        auto model = static_cast<IQISCADModelScriptEdit*>( modelTabs_->widget(i) );
        if (model) model->closeEvent(event);
    }
    
    
    if (event->isAccepted())
    {
        QSettings settings("silentdynamics", "iscad");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
    }
}



    
void IQISCADMainWindow::updateProgress(int step, int totalSteps)
{
  progressbar_->setMaximum(totalSteps);
  progressbar_->setValue(step);
}


IQISCADModelWindow* IQISCADMainWindow::insertEmptyModel(bool bgparsing)
{
    auto *me = new IQISCADModelWindow();

  int idx = modelTabs_->addTab(me, QString("(")+_("unnamed")+")");
    modelTabs_->setCurrentIndex(idx);
    
    connect(me->modelEdit(), &IQISCADModelScriptEdit::displayStatusMessage,
            statusBar(), &QStatusBar::showMessage);
//    connect(me->viewer(), &QoccViewWidget::sendStatus,
//            statusBar(), &QStatusBar::showMessage);
    connect(me->modelEdit(), &IQISCADModelScriptEdit::statusProgress,
            this, &IQISCADMainWindow::updateProgress);

    connect(me, &IQISCADModelWindow::updateTabTitle,
            this, &IQISCADMainWindow::onUpdateTabTitle);

    return me;
}

IQISCADModelWindow* IQISCADMainWindow::insertModel(const boost::filesystem::path& file, bool bgparsing)
{
    auto* me = insertEmptyModel(bgparsing);
    me->modelEdit()->loadFile(file);
    me->modelEdit()->unsetUnsavedState();
    return me;
}

IQISCADModelWindow* IQISCADMainWindow::insertModelScript(const std::string& contents, bool bgparsing)
{
    auto* me = insertEmptyModel(bgparsing);
    me->modelEdit()->setScript(contents);
    me->modelEdit()->unsetUnsavedState();
    return me;
}

    
void IQISCADMainWindow::onFileClicked(const QModelIndex &index)
{
    insertModel( fileModel_->filePath(index).toStdString() );
}

void IQISCADMainWindow::onCreateNewModel(const QString& directory)
{
    QString fn=QFileDialog::getSaveFileName
    (
        this,
        _("Select file"),
        directory,
        _("ISCAD Model Files (*.iscad)")
    );
    if (fn!="")
    {
        IQISCADModelWindow *me=insertEmptyModel();
        me->modelEdit()->setFilename(qPrintable(fn));
        me->modelEdit()->saveModel();
    }
}


void IQISCADMainWindow::onDeleteModel(const QString& filepath)
{
    QFileInfo fi(filepath);
   
    QMessageBox::StandardButton resBtn = QMessageBox::Yes;

    resBtn =
        QMessageBox::question
        (
            this,
            "ISCAD",
            QString(_("Really delete file %1?\n")).arg(fi.baseName()),
            QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
            QMessageBox::No
        );

    if (resBtn == QMessageBox::Yes)
    {
        QFile::remove(filepath);
    }
}


void IQISCADMainWindow::onShowFileTreeContextMenu(const QPoint& p)
{
    QMenu myMenu;
    QAction *a;
    QSignalMapper* mapper;
    
    QFileInfo fi=fileModel_->fileInfo(fileTree_->currentIndex());
    
    a=new QAction(_("Create new file..."), &myMenu);

    QString dir;
    if (fi.isDir())
        dir=fi.absoluteFilePath();
    else
        dir=fi.absolutePath();

    connect(a, &QAction::triggered,
            [this, dir]()
            {
              this->onCreateNewModel(dir);
            }
    );

    myMenu.addAction(a);

    if (!fi.isDir())
    {
        a=new QAction(QString(_("Delete file %1")).arg(fi.baseName()), &myMenu);
        mapper = new QSignalMapper(a) ;
        connect(a, &QAction::triggered,
                [this, fi]()
                {
                  this->onDeleteModel(fi.absoluteFilePath());
                }
                );

        myMenu.addAction(a);
    }

    myMenu.exec(fileTree_->mapToGlobal(p));
}
