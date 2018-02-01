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

#include "iscadmainwindow.h"
#include "iscadmodel.h"

#include "qmodeltree.h"
#include "qmodelstepitem.h"
#include "qvariableitem.h"
#include "qdatumitem.h"
#include "qevaluationitem.h"
#include "iscadsyntaxhighlighter.h"
#include "occtools.h"

#include "base/boost_include.h"

#include <QSignalMapper>

#include "modelfeature.h"
#include "modelcomponentselectordlg.h"
#include "insertfeaturedlg.h"

#include "datum.h"


 

void ISCADMainWindow::connectMenuToModel(ISCADModelEditor* me, ISCADModelEditor* lme)
{
    act_load_->disconnect();
    act_save_->disconnect();
    act_saveas_->disconnect();
    act_rebuild_->disconnect();
    act_rebuild_UTC_->disconnect();
    act_insert_section_comment_->disconnect();
    act_insert_feat_->disconnect();
    act_insert_component_name_->disconnect();
    act_clear_cache_->disconnect();
    act_fit_all_->disconnect();
    act_toggle_grid_->disconnect();
    act_toggle_clipxy_->disconnect();
    act_toggle_clipyz_->disconnect();
    act_toggle_clipxz_->disconnect();
    act_background_color_->disconnect();
    act_display_all_shaded_->disconnect();
    act_display_all_wire_->disconnect();
    act_reset_shading_->disconnect();

    act_measure_distance_->disconnect();
    act_sel_pts_->disconnect();
    act_sel_edgs_->disconnect();
    act_sel_faces_->disconnect();

    if (lme!=NULL)
      {
        disconnect(lme->model(), SIGNAL(modelUpdated(int)),
                   this, SLOT(onUpdateClipPlaneMenu()));
        disconnect(lme->model(), SIGNAL(openModel(const boost::filesystem::path&)),
                   this, SLOT(onLoadModelFile(const boost::filesystem::path&)));
      }

    bgparsestopbtn_->disconnect(SIGNAL(clicked()));

    if (me)
    {
        connect(act_save_, SIGNAL(triggered()),
                me->model(), SLOT(saveModel()));
        connect(act_saveas_, SIGNAL(triggered()),
                me->model(), SLOT(saveModelAs()));
        connect(act_rebuild_, SIGNAL(triggered()),
                me->model(), SLOT(rebuildModel()));
        connect(act_rebuild_UTC_, SIGNAL(triggered()),
                me->model(), SLOT(rebuildModelUpToCursor()));
        connect(act_insert_section_comment_, SIGNAL(triggered()),
                me->model(), SLOT(insertSectionCommentAtCursor()));
        connect(act_insert_feat_, SIGNAL(triggered()),
                me->model(), SLOT(insertFeatureAtCursor()));
        connect(act_insert_component_name_, SIGNAL(triggered()),
                me->model(), SLOT(insertComponentNameAtCursor()));
        connect(act_clear_cache_, SIGNAL(triggered()),
                me->model(), SLOT(clearCache()));

        connect(act_fit_all_, SIGNAL(triggered()),
                me->viewer(), SLOT(fitAll()));
        connect(act_toggle_grid_, SIGNAL(triggered()),
                me->viewer(), SLOT(toggleGrid()));
        connect(act_toggle_clipxy_, SIGNAL(triggered()),
                me->viewer(), SLOT(toggleClipXY()));
        connect(act_toggle_clipyz_, SIGNAL(triggered()),
                me->viewer(), SLOT(toggleClipYZ()));
        connect(act_toggle_clipxz_, SIGNAL(triggered()),
                me->viewer(), SLOT(toggleClipXZ()));
        connect(act_background_color_, SIGNAL(triggered()),
                me->viewer(), SLOT(background()));
        connect(act_display_all_shaded_, SIGNAL(triggered()),
                me->modeltree(), SLOT(allShaded()));
        connect(act_display_all_wire_, SIGNAL(triggered()),
                me->modeltree(), SLOT(allWireframe()));
        connect(act_reset_shading_, SIGNAL(triggered()),
                me->modeltree(), SLOT(resetViz()));

        connect(act_measure_distance_, SIGNAL(triggered()),
                me->viewer(), SLOT(onMeasureDistance()));

        connect(act_sel_pts_, SIGNAL(triggered()),
                me->viewer(), SLOT(onSelectPoints()));

        connect(act_sel_edgs_, SIGNAL(triggered()),
                me->viewer(), SLOT(onSelectEdges()));

        connect(act_sel_faces_, SIGNAL(triggered()),
                me->viewer(), SLOT(onSelectFaces()));

        me->model()->populateClipPlaneMenu(clipplanemenu_, me->viewer());

        connect(bgparsestopbtn_, SIGNAL(clicked()),
                me->model(), SLOT(onCancelRebuild()));

        connect(me->model(), SIGNAL(modelUpdated(int)),
                this, SLOT(onUpdateClipPlaneMenu(int)));
        
        connect(me->model(), SIGNAL(openModel(const boost::filesystem::path&)),
                this, SLOT(onLoadModelFile(const boost::filesystem::path&)));

      }
}



void ISCADMainWindow::loadModel()
{
    QString fn=QFileDialog::getOpenFileName(this, "Select file", "", "ISCAD Model Files (*.iscad)");
    if (fn!="")
    {
        insertModel(qPrintable(fn))->model()->unsetUnsavedState();
    }
}


void ISCADMainWindow::activateModel(int tabindex)
{
    if ( (tabindex>=0) && (tabindex!=lastTabIndex_) )
    {
        ISCADModelEditor* lme=NULL;
        if (lastTabIndex_>=0) lme=static_cast<ISCADModelEditor*>(modelTabs_->widget(lastTabIndex_));

        connectMenuToModel(static_cast<ISCADModelEditor*>(modelTabs_->widget(tabindex)), lme);
        lastTabIndex_=tabindex;
    }
}

// void ISCADMainWindow::deactivateModel(ISCADModel* model)
// {
// }


void ISCADMainWindow::onUpdateTabTitle(ISCADModelEditor* model, const boost::filesystem::path& filepath, bool isUnSaved)
{
    int i=modelTabs_->indexOf(model);
    if (i>=0)
    {
        modelTabs_->setTabText(i, QString(isUnSaved?"*":"") + QString(filepath.filename().c_str()));
        modelTabs_->setTabToolTip(i, QString(filepath.c_str()));
    }
}

void ISCADMainWindow::onCloseModel(int tabindex)
{
    ISCADModel *model = static_cast<ISCADModel*>(modelTabs_->widget(tabindex));
    if (model)
    {
        QCloseEvent ev;
        model->closeEvent(&ev);
        if (ev.isAccepted())
            modelTabs_->removeTab(tabindex);
    }
}

void ISCADMainWindow::onUpdateClipPlaneMenu(int errorState)
{
  if (errorState==0)
    {
      if (ISCADModelEditor *me = static_cast<ISCADModelEditor*>(modelTabs_->currentWidget()))
      {
          me->model()->populateClipPlaneMenu(clipplanemenu_, me->viewer());
      }
    }
}

void ISCADMainWindow::onNewModel()
{
  insertEmptyModel();
}

void ISCADMainWindow::onLoadModelFile(const boost::filesystem::path& modelfile)
{
    insertModel(modelfile);
}


ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags, bool nolog)
: QMainWindow(parent, flags),
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
      logger_=new Q_DebugStream(std::cout); // ceases to work with multithreaded bg parsing
      connect(logger_, SIGNAL(appendText(const QString&)),
              log_, SLOT(append(const QString&)));

    }
    
    spl0->addWidget(log_);
    
    const QString rootPath = QDir::currentPath();
    fileModel_ = new QFileSystemModel;
    fileTree_=new QTreeView;
    fileTree_->setModel( fileModel_ );
    QStringList filter;
    filter << "*.iscad";
    fileModel_->setNameFilters( filter );
    fileModel_->setNameFilterDisables(false);
    fileModel_->setRootPath( rootPath );
    fileTree_->setRootIndex( fileModel_->index( rootPath ) );
    fileTree_->expandAll();
    for (int i = 1; i < fileModel_->columnCount(); ++i)
        fileTree_->hideColumn(i);
    connect(fileTree_, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(onFileClicked(const QModelIndex &)));
    fileTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTree_, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(onShowFileTreeContextMenu(QPoint)));
    spl->addWidget(fileTree_);
    
    modelTabs_=new QTabWidget;
    modelTabs_->setTabsClosable(true);
    spl->addWidget(modelTabs_);
    connect(modelTabs_, SIGNAL(currentChanged(int)), this, SLOT(activateModel(int)));
    connect(modelTabs_, SIGNAL(tabCloseRequested(int)), this, SLOT(onCloseModel(int)));

    {
      QList<int> sizes;
      sizes << 1500 << 8500;
      spl->setSizes(sizes);
    }

    {
      QList<int> sizes;
      sizes << 9500 << 500;
      spl0->setSizes(sizes);
    }

    progressbar_=new QProgressBar;
    bgparsestopbtn_=new QPushButton("STOP");
    statusBar()->addPermanentWidget(progressbar_);
    statusBar()->addPermanentWidget(bgparsestopbtn_);

    QMenu *fmenu = menuBar()->addMenu("&File");
    QMenu *mmenu = menuBar()->addMenu("&Model");
    QMenu *vmenu = menuBar()->addMenu("&View");
    QMenu *msmenu = menuBar()->addMenu("M&easure");

    QAction *act;

    act=new QAction(("&New"), this);
    fmenu->addAction(act);
    connect(act, SIGNAL(triggered()), this, SLOT(onNewModel()));

    act_load_=new QAction(("&Load"), this);
    fmenu->addAction(act_load_);
    connect(act_load_, SIGNAL(triggered()), this, SLOT(loadModel()));

    act_save_ = new QAction(("&Save"), this);
    act_save_->setShortcut(Qt::ControlModifier + Qt::Key_S);
    fmenu->addAction(act_save_);

    act_saveas_ = new QAction(("&Save as..."), this);
    fmenu->addAction(act_saveas_);

    fmenu->addSeparator();
    act =new QAction(("&Quit"), this);
    act->setShortcut(Qt::AltModifier + Qt::Key_F4);
    fmenu->addAction(act);
    connect(act, SIGNAL(triggered()), this, SLOT(close()));


    act_rebuild_ = new QAction(("&Rebuild model"), this);
    act_rebuild_->setShortcut(Qt::ControlModifier + Qt::Key_Return);
    mmenu->addAction(act_rebuild_);

    act_rebuild_UTC_ = new QAction(("&Rebuild model up to cursor"), this);
    act_rebuild_UTC_->setShortcut(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_Return);
    mmenu->addAction(act_rebuild_UTC_);
    
    act_insert_feat_ = new QAction(("Insert &feature..."), this);
    act_insert_feat_->setShortcut(Qt::ControlModifier + Qt::Key_F);
    mmenu->addAction(act_insert_feat_);

    act_insert_component_name_ = new QAction(("Insert &component name..."), this);
    act_insert_component_name_->setShortcut(Qt::ControlModifier + Qt::Key_I);
    mmenu->addAction(act_insert_component_name_);

    
    act_insert_section_comment_ = new QAction(("Insert comment: new section..."), this);
    act_insert_section_comment_->setShortcut(Qt::AltModifier + Qt::Key_S);
    mmenu->addAction(act_insert_section_comment_);

    act_clear_cache_ = new QAction(("C&lear cache"), this);
    act_clear_cache_->setShortcut(Qt::ControlModifier + Qt::Key_L);
    mmenu->addAction(act_clear_cache_);

    act_fit_all_ = new QAction(("Fit &all"), this);
    act_fit_all_->setShortcut(Qt::ControlModifier + Qt::Key_A);
    vmenu->addAction(act_fit_all_);
    act_toggle_grid_ = new QAction(("Toggle &grid"), this);
    vmenu->addAction(act_toggle_grid_);
    
    act_toggle_clipxy_ = new QAction(("Toggle clip plane (&XY)"), this);
    vmenu->addAction(act_toggle_clipxy_);
    act_toggle_clipyz_ = new QAction(("Toggle clip plane (&YZ)"), this);
    vmenu->addAction(act_toggle_clipyz_);
    act_toggle_clipxz_ = new QAction(("Toggle clip plane (X&Z)"), this);
    vmenu->addAction(act_toggle_clipxz_);
    
    clipplanemenu_=vmenu->addMenu("Clip at datum plane");
    clipplanemenu_->setDisabled(true);
    
    act_background_color_ = new QAction(("Change background color..."), this);
    vmenu->addAction(act_background_color_);
    act_display_all_shaded_ = new QAction(("Display all &shaded"), this);
    vmenu->addAction(act_display_all_shaded_);
    act_display_all_wire_ = new QAction(("Display all &wireframe"), this);
    vmenu->addAction(act_display_all_wire_);
    act_reset_shading_ = new QAction(("&Reset shading and visibility"), this);
//     act->setShortcut(Qt::ControlModifier + Qt::Key_A);
    vmenu->addAction(act_reset_shading_);


    act_measure_distance_=new QAction("Distance between points", this);
    act_measure_distance_->setShortcut(Qt::ControlModifier + Qt::Key_M);
    msmenu->addAction(act_measure_distance_);

    act_sel_pts_=new QAction("Select vertices", this);
    msmenu->addAction(act_sel_pts_);

    act_sel_edgs_=new QAction("Select edges", this);
    msmenu->addAction(act_sel_edgs_);

    act_sel_faces_=new QAction("Select faces", this);
    msmenu->addAction(act_sel_faces_);

    QSettings settings;
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());

 
}

ISCADMainWindow::~ISCADMainWindow()
{
}




void ISCADMainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    
    for (int i=0; i<modelTabs_->count(); i++)
    {
        ISCADModel *model = static_cast<ISCADModel*>( modelTabs_->widget(i) );
        if (model) model->closeEvent(event);
    }
    
    
    if (event->isAccepted())
    {
        QSettings settings;
        settings.setValue("mainWindowGeometry", saveGeometry());
        settings.setValue("mainWindowState", saveState());
    }
}



    
void ISCADMainWindow::updateProgress(int step, int totalSteps)
{
  progressbar_->setMaximum(totalSteps);
  progressbar_->setValue(step);
}


ISCADModelEditor* ISCADMainWindow::insertEmptyModel(bool bgparsing)
{
    ISCADModelEditor *me = new ISCADModelEditor();
    modelTabs_->addTab(me, "(unnamed)");
    
    connect(me->model(), SIGNAL(displayStatusMessage(const QString&)),
            statusBar(), SLOT(showMessage(const QString&)));
    connect(me->viewer(), SIGNAL(sendStatus(const QString&)),
            statusBar(), SLOT(showMessage(const QString&)));
    connect(me->model(), SIGNAL(statusProgress(int, int)),
            this, SLOT(updateProgress(int, int)));

    connect(me, SIGNAL(updateTabTitle(ISCADModelEditor*, const boost::filesystem::path&, bool)),
            this, SLOT(onUpdateTabTitle(ISCADModelEditor*, const boost::filesystem::path&, bool)));

    return me;
}

ISCADModelEditor* ISCADMainWindow::insertModel(const boost::filesystem::path& file, bool bgparsing)
{
    ISCADModelEditor* me = insertEmptyModel(bgparsing);
    me->model()->loadFile(file);
    me->model()->unsetUnsavedState();
    return me;
}

ISCADModelEditor* ISCADMainWindow::insertModelScript(const std::string& contents, bool bgparsing)
{
    ISCADModelEditor* me = insertEmptyModel(bgparsing);
    me->model()->setScript(contents);
    me->model()->unsetUnsavedState();
    return me;
}

    
void ISCADMainWindow::onFileClicked(const QModelIndex &index)
{
    insertModel( fileModel_->filePath(index).toStdString() );
}

void ISCADMainWindow::onCreateNewModel(const QString& directory)
{
    QString fn=QFileDialog::getSaveFileName
    (
        this, 
        "Select file", 
        directory, 
        "ISCAD Model Files (*.iscad)"
    );
    if (fn!="")
    {
        ISCADModelEditor *me=insertEmptyModel();
        me->model()->setFilename(qPrintable(fn));
        me->model()->saveModel();
    }
}


void ISCADMainWindow::onDeleteModel(const QString& filepath)
{
    QFileInfo fi(filepath);
   
    QMessageBox::StandardButton resBtn = QMessageBox::Yes;

    resBtn =
        QMessageBox::question
        (
            this,
            "ISCAD",
            QString("Really delete file ")+fi.baseName()+"?\n",
            QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
            QMessageBox::No
        );

    if (resBtn == QMessageBox::Yes)
    {
        QFile::remove(filepath);
    }
}


void ISCADMainWindow::onShowFileTreeContextMenu(const QPoint& p)
{
    QMenu myMenu;
    QAction *a;
    QSignalMapper* mapper;
    
    QFileInfo fi=fileModel_->fileInfo(fileTree_->currentIndex());
    
    a=new QAction("Create new file...", &myMenu);
    mapper = new QSignalMapper(a) ;
    connect(a, SIGNAL(triggered()), mapper, SLOT(map())) ;
    QString dir;
    if (fi.isDir())
        dir=fi.absoluteFilePath();
    else
        dir=fi.absolutePath();
    mapper->setMapping(a, dir);
    connect(mapper, SIGNAL(mapped(const QString &)),
            this, SLOT(onCreateNewModel(const QString&)));
    myMenu.addAction(a);

    if (!fi.isDir())
    {
        a=new QAction("Delete file "+fi.baseName(), &myMenu);
        mapper = new QSignalMapper(a) ;
        connect(a, SIGNAL(triggered()), mapper, SLOT(map())) ;
        mapper->setMapping(a, fi.absoluteFilePath());
        connect(mapper, SIGNAL(mapped(const QString &)),
                this, SLOT(onDeleteModel(const QString&)));
        myMenu.addAction(a);
    }

    myMenu.exec(fileTree_->mapToGlobal(p));
}
