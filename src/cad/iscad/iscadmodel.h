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
#include <QSplitter>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>

#include "qoccviewercontext.h"
#include "qoccviewwidget.h"
#include "qdebugstream.h"
#include "viewstate.h"
#include "qmodeltree.h"
#include "bgparsingthread.h"

#ifndef Q_MOC_RUN
#include "pointertransient.h"
#include "cadfeaturetransient.h"
#include "occinclude.h"
#include "cadfeature.h"
#include "parser.h"
#include "sketch.h"
#include "cadmodel.h"
#endif




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

    QTimer *bgparseTimer_;
    const int bgparseInterval=1000;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    bool unsaved_;
    bool doBgParsing_;
    
    insight::cad::ModelPtr cur_model_;
    
    BGParsingThread bgparsethread_;
    
    bool skipPostprocActions_;

    QSize sizehint_;

    
protected:
    void clearDerivedData();

    inline void setFilename(const boost::filesystem::path& fn)
    {
        filename_=fn;
        emit updateTitle(filename_, false);
    }

    
public:
    ISCADModel(QWidget* parent = 0, bool dobgparsing=true);
    ~ISCADModel();
    
    void loadFile(const boost::filesystem::path& file);
    void setScript(const std::string& contents);
    
    /**
     * add all defined planes to the clip plane menu
     */
    void populateClipPlaneMenu(QMenu* clipplanemenu, QoccViewWidget* v);

    void connectModelTree(QModelTree* mt) const;

    inline bool isUnsaved() const { return unsaved_; }

    virtual QSize sizeHint() const;

public slots:
    void onGraphicalSelectionChanged(QoccViewWidget* aView);

    /**
     * some text has been selected. Highlight all identical occurrences
     */
    void onEditorSelectionChanged();

    /**
     * jump to definition of feature symbol
     */
    void jumpTo(const QString& featurename);

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
    
//    /**
//     * display everything shaded
//     */
//    void allShaded();
    
//    /**
//     * display everything in wireframe
//     */
//    void allWireframe();

    
public Q_SLOTS:

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
    void showEditorContextMenu(const QPoint&);

    /**
     * insert some text at the cursor
     */
    void insertTextAtCursor(const QString& snippet);


    void setUnsavedState(int i1=0, int i2=1, int i3=1);
    void unsetUnsavedState();

    void onScriptError(long failpos, QString errorMsg, int range=1);

    void onCancelRebuild();

Q_SIGNALS:
    
    /**
     * user status informations
     */
    void displayStatusMessage(const QString&, double timeout=0);

    /**
     * @brief statusProgress
     * @param step
     * @param totalSteps
     * forwarded from parsing thread
     */
    void statusProgress(int step, int totalSteps);

    /**
     * change of model file name or save state (asterisk in front of name)
     */
    void updateTitle(const boost::filesystem::path& filepath, bool isUnSaved);
    
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

    /**
     * @brief clearData
     * clear all model data (e.g., before a new script is inserted during file load)
     */
    void clearData();

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
    QTextEdit* notepad_;

public:
    ISCADModelEditor(QWidget* parent = 0);

    inline ISCADModel* model() { return model_; }
    inline QoccViewWidget* viewer() { return viewer_; }
    inline QTextEdit* notepad() { return notepad_; }
    inline QModelTree* modeltree() { return modeltree_; }

public Q_SLOTS:
    void onCopyBtnClicked();
    void onUpdateTitle(const boost::filesystem::path& filepath, bool isUnSaved);
    void onInsertNotebookText(const QString& text);

protected:
    virtual void closeEvent(QCloseEvent *event);

signals:
    void updateTabTitle(ISCADModelEditor* model, const boost::filesystem::path& filepath, bool isUnSaved);
};


#endif
