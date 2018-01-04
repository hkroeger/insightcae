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

#ifndef INSIGHT_CAD_ISCADMODEL_H
#define INSIGHT_CAD_ISCADMODEL_H

#include <QMainWindow>
#include <QTimer>
#include <QTreeView>
#include <QFileSystemModel>
#include <QTabWidget>

#include "qoccviewercontext.h"
#include "qoccviewwidget.h"
#include "qdebugstream.h"
#include "viewstate.h"
#include "qmodeltree.h"

#ifndef Q_MOC_RUN
#include "pointertransient.h"
#include "cadfeaturetransient.h"
#include "occinclude.h"
#include "cadfeature.h"
#include "parser.h"
#include "sketch.h"
#include "cadmodel.h"
#endif




class ISCADSyntaxHighlighter;
class ISCADMainWindow;


/**
 * the parsing and rebuilding processor.
 * To be started in a separate thread to keep GUI responsive
 */
class BGParsingThread
: public QThread
{
    Q_OBJECT
    
public:
    enum Action { ParseOnly, ParseAndRebuild };
    
protected:
    std::string script_;
    Action action_;
    
public:
    insight::cad::ModelPtr model_;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    
    /**
     * is created upon model construction
     */
    BGParsingThread();
    
    /**
     * restarts the actions
     */
    void launch(const std::string& script, Action act = ParseOnly);
    virtual void run();
    void extendActionToRebuild();
    
signals:
    void addFeature(QString sn, insight::cad::FeaturePtr sm, bool is_component);
    void addDatum(QString sn, insight::cad::DatumPtr dm);
    void addEvaluation(QString sn, insight::cad::PostprocActionPtr em, bool visible=false);
    void addVariable(QString sn, insight::cad::parser::scalar sv);
    void addVariable(QString sn, insight::cad::parser::vector vv);

    void scriptError(int failpos, QString errorMsg);
    
    void statusMessage(QString msg);
    void statusProgress(int step, int totalSteps);
};





/**
 * the container for the CAD modeling data, is also the text editor widget
 */
class ISCADModel
: public QTextEdit
{
    Q_OBJECT
    
    friend class ISCADMainWindow;
    
protected:
    boost::filesystem::path filename_;
    ISCADSyntaxHighlighter* highlighter_;

//     std::map<std::string, ViewState> checked_modelsteps_, checked_datums_, checked_evaluations_;

    std::vector<Handle_AIS_InteractiveObject> additionalDisplayObjectsForSelection_;

    QTimer *bgparseTimer_;
    const int bgparseInterval=1000;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    bool unsaved_;
    bool doBgParsing_;
    
    insight::cad::ModelPtr cur_model_;
    
    BGParsingThread bgparsethread_;
    
    bool skipPostprocActions_;

    QTextEdit* notepad_;
    
protected:
    void clearDerivedData();
    virtual void closeEvent(QCloseEvent *event);

    inline void setFilename(const boost::filesystem::path& fn)
    {
        filename_=fn;
        emit updateTabTitle(this, filename_, false);
    }

    template<class PT>
    PT* checkGraphicalSelection(QoccViewWidget* aView)
    {
        if (aView->getContext()->HasDetected())
        {
            if (aView->getContext()->DetectedInteractive()->HasOwner())
            {
                Handle_Standard_Transient own=aView->getContext()->DetectedInteractive()->GetOwner();
                if (!own.IsNull())
                {
                    if (PointerTransient *smo=dynamic_cast<PointerTransient*>(own
#if (OCC_VERSION_MAJOR<7)
                        .Access()
#else
                        .get()
#endif
                    ))
                    {
                        if (PT* mi=dynamic_cast<PT*>(smo->getPointer()))
                        {
                            return mi;
                        }
                    }
                }
            }
        }
        return NULL;
    }
    
public:
    ISCADModel(QWidget* parent = 0, bool dobgparsing=true);
    ~ISCADModel();
    
    void loadFile(const boost::filesystem::path& file);
    void setScript(const std::string& contents);
    
    /**
     * add all defined planes to the clip plane menu
     */
    void populateClipPlaneMenu(QMenu* clipplanemenu);

protected slots:
    void onGraphicalSelectionChanged(QoccViewWidget* aView);
//     void onModelTreeItemChanged(QTreeWidgetItem * item, int);

    /**
     * some text has been selected. Highlight all identical occurrences
     */
    void onEditorSelectionChanged();

    /**
     * jump to definition of feature symbol
     */
    void jump_to(const QString& featurename);

    /**
     * switch BG parsing on/off
     */
    void toggleBgParsing(int state);

    /**
     * trigger countdown timer for background parsing
     */
    void restartBgParseTimer(int i1=0,int i2=0,int i3=0);
    
    /**
     * execute background parsing
     */
    void doBgParse();

    /**
     * BG parsing thread has finished
     */
    void onBgParseFinished();

    /**
     * execute sketch editor for selected insight::cad::Sketch
     * triggered from QAction (menu) through a signalMapper
     */
    void editSketch(QObject* sk_ptr);
    
    /**
     * open another insight::cad::ModelFeature in a new tab
     * triggered from QAction (menu) through a signalMapper
     */
    void editModel(QObject* mo_ptr);

    /**
     * switch evaluation of Postproc Actions on/off
     */
    void toggleSkipPostprocActions(int state);
    
    void insertSectionCommentAtCursor();
    void insertFeatureAtCursor();
    void insertComponentNameAtCursor();
    void onCopyBtnClicked();
    
    /**
     * display everything shaded
     */
    void allShaded();
    
    /**
     * display everything in wireframe
     */
    void allWireframe();
    
    void onSetClipPlane(QObject* datumplane);
    
public slots:

    /**
     * save model under the file name from which is was loaded
     */
    bool saveModel();
    
    /**
     * save under new file name. Dialog will be displayed
     */
    bool saveModelAs();
    
    /**
     * trigger rebuilding of entire model
     */
    void rebuildModel(bool upToCursor=false);
    
    /**
     * rebuild only up to cursor
     */
    void rebuildModelUpToCursor();
    
    /**
     * clear the cache
     */
    void clearCache();
    
    /**
     * create a context menu in editor widget
     */
    void popupMenu( QoccViewWidget* aView, const QPoint aPoint );
    void showEditorContextMenu(const QPoint&);

    void setUnsavedState(int i1=0, int i2=1, int i3=1);
    void unsetUnsavedState();
    
signals:
    
    /**
     * user status informations
     */
    void displayStatusMessage(const QString&);
    
    /**
     * change of model file name or save state (asterisk in front of name)
     */
    void updateTabTitle(ISCADModel* model, const boost::filesystem::path& filepath, bool isUnSaved);
    
    /**
     * the model has been reevaluated and menu entries etc. may have changed:
     * 
     * - clip planes
     * 
     * errorState != 0 indicates that an error occurred and the model may be incomplete
     */
    void modelUpdated(int errorState =0);
    
    /**
     * open another model for editing
     */
    void openModel(const boost::filesystem::path& modelfile);

//     // insert new features
//     void addFeature(const QString& sn, insight::cad::FeaturePtr sm, bool is_component);
//     void addDatum(const QString& sn, insight::cad::DatumPtr dm);
//     void addEvaluation(const QString& sn, insight::cad::PostprocActionPtr em, bool visible=false);
//     void addVariable(const QString& sn, insight::cad::parser::scalar sv);
//     void addVariable(const QString& sn, insight::cad::parser::vector vv);

};



/**
 * a widget, which arranges all widgets, required for editing a model:
 * text editor, graphical window, model tree and buttons
 */
class ISCADModelEditor
: public QWidget
{
    Q_OBJECT
protected:
    QoccViewerContext* context_;
    QoccViewWidget* viewer_;
    QModelTree* modeltree_;
    ISCADModel* model_;

public:
    ISCADModelEditor(QWidget* parent = 0);
};


#endif
