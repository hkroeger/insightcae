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


 

void ISCADMainWindow::connectMenuToModel(ISCADModel* model)
{
    act_load_->disconnect();
    act_save_->disconnect();
    act_saveas_->disconnect();
    act_rebuild_->disconnect();
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
    
    disconnect(this, SLOT(onUpdateClipPlaneMenu()));
    disconnect(this, SLOT(onLoadModelFile(const boost::filesystem::path&)));
        
    if (model)
    {
        connect(act_save_, SIGNAL(triggered()), model, SLOT(saveModel()));
        connect(act_saveas_, SIGNAL(triggered()), model, SLOT(saveModelAs()));
        connect(act_rebuild_, SIGNAL(triggered()), model, SLOT(rebuildModel()));
        connect(act_insert_feat_, SIGNAL(triggered()), model, SLOT(insertFeatureAtCursor()));
        connect(act_insert_component_name_, SIGNAL(triggered()), model, SLOT(insertComponentNameAtCursor()));
        connect(act_clear_cache_, SIGNAL(triggered()), model, SLOT(clearCache()));
        connect(act_fit_all_, SIGNAL(triggered()), model->viewer_, SLOT(fitAll()));
        connect(act_toggle_grid_, SIGNAL(triggered()), model->context_, SLOT(toggleGrid()));
        connect(act_toggle_clipxy_, SIGNAL(triggered()), model->viewer_, SLOT(toggleClipXY()));
        connect(act_toggle_clipyz_, SIGNAL(triggered()), model->viewer_, SLOT(toggleClipYZ()));
        connect(act_toggle_clipxz_, SIGNAL(triggered()), model->viewer_, SLOT(toggleClipXZ()));
        connect(act_background_color_, SIGNAL(triggered()), model->viewer_, SLOT(background()));
        connect(act_display_all_shaded_, SIGNAL(triggered()), model, SLOT(allShaded()));
        connect(act_display_all_wire_, SIGNAL(triggered()), model, SLOT(allWireframe()));
        connect(act_reset_shading_, SIGNAL(triggered()), model->modeltree_, SLOT(resetViz()));
        
        model->populateClipPlaneMenu(clipplanemenu_);
        connect(model, SIGNAL(updateClipPlaneMenu()), this, SLOT(onUpdateClipPlaneMenu()));
        
        connect(model, SIGNAL(openModel(const boost::filesystem::path&)), 
                this, SLOT(onLoadModelFile(const boost::filesystem::path&)));
    }
}



void ISCADMainWindow::loadModel()
{
    QString fn=QFileDialog::getOpenFileName(this, "Select file", "", "ISCAD Model Files (*.iscad)");
    if (fn!="")
    {
        insertModel(qPrintable(fn))->unsetUnsavedState();
    }
}


void ISCADMainWindow::activateModel(int tabindex)
{
    if (tabindex>=0)
    {
        connectMenuToModel(static_cast<ISCADModel*>(modelTabs_->widget(tabindex)));
    }
}

// void ISCADMainWindow::deactivateModel(ISCADModel* model)
// {
// }


void ISCADMainWindow::onUpdateTabTitle(ISCADModel* model, const boost::filesystem::path& filepath, bool isUnSaved)
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

void ISCADMainWindow::onUpdateClipPlaneMenu()
{
    ISCADModel *model = static_cast<ISCADModel*>(modelTabs_->currentWidget());
    if (model)
    {
        model->populateClipPlaneMenu(clipplanemenu_);
    }
}

void ISCADMainWindow::onLoadModelFile(const boost::filesystem::path& modelfile)
{
    insertModel(modelfile);
}


ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags, bool nolog)
: QMainWindow(parent, flags)
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
      connect(logger_, SIGNAL(appendText(const QString&)), log_, SLOT(append(const QString&)));
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

    QList<int> sizes;
    sizes << 50 << 500+350+150;
    spl->setSizes(sizes);

    QMenu *fmenu = menuBar()->addMenu("&File");
    QMenu *mmenu = menuBar()->addMenu("&Model");
    QMenu *vmenu = menuBar()->addMenu("&View");

    act_load_=new QAction(("&Load"), this);
    fmenu->addAction(act_load_);
    connect(act_load_, SIGNAL(triggered()), this, SLOT(loadModel()));

    act_save_ = new QAction(("&Save"), this);
    act_save_->setShortcut(Qt::ControlModifier + Qt::Key_S);
    fmenu->addAction(act_save_);

    act_saveas_ = new QAction(("&Save as..."), this);
    fmenu->addAction(act_saveas_);

    act_rebuild_ = new QAction(("&Rebuild model"), this);
    act_rebuild_->setShortcut(Qt::ControlModifier + Qt::Key_Return);
    mmenu->addAction(act_rebuild_);

    act_insert_feat_ = new QAction(("Insert &feature..."), this);
    act_insert_feat_->setShortcut(Qt::ControlModifier + Qt::Key_F);
    mmenu->addAction(act_insert_feat_);

    act_insert_component_name_ = new QAction(("Insert &component name..."), this);
    act_insert_component_name_->setShortcut(Qt::ControlModifier + Qt::Key_I);
    mmenu->addAction(act_insert_component_name_);

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



    



ISCADModel* ISCADMainWindow::insertEmptyModel()
{
    ISCADModel *model = new ISCADModel;
    modelTabs_->addTab(model, "(unnamed)");
    
    connect(model, SIGNAL(displayStatus(const QString&)), this, SLOT(displayStatusMessage(const QString&)));
    connect(model, SIGNAL(updateTabTitle(ISCADModel*, const boost::filesystem::path&, bool)), 
            this, SLOT(onUpdateTabTitle(ISCADModel*, const boost::filesystem::path&, bool)));

    return model;
}

ISCADModel* ISCADMainWindow::insertModel(const boost::filesystem::path& file)
{
    ISCADModel* model = insertEmptyModel();
    model->loadFile(file);
    return model;
}

ISCADModel* ISCADMainWindow::insertModelScript(const std::string& contents)
{
    ISCADModel* model = insertEmptyModel();
    model->setScript(contents);
    return model;
}

    
void ISCADMainWindow::onFileClicked(const QModelIndex &index)
{
    insertModel( fileModel_->filePath(index).toStdString() );
}

void ISCADMainWindow::displayStatusMessage(const QString& message)
{
    statusBar()->showMessage(message);
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
        ISCADModel *model=insertEmptyModel();
        model->setFilename(qPrintable(fn));
        model->saveModel();
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
