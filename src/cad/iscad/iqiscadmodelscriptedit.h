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

#include <QTextEdit>

#include "base/boost_include.h"
#include "iqiscadbackgroundthread.h"


#ifndef Q_MOC_RUN
#include "parser.h"
#endif


class IQCADItemModel;
class IQISCADSyntaxHighlighter;



/**
 * the container for the CAD modeling data, is also the text editor widget
 */
class IQISCADModelScriptEdit
: public QTextEdit
{
    Q_OBJECT
    
    friend class IQISCADMainWindow;
    
protected:
    boost::filesystem::path filename_;
    IQISCADSyntaxHighlighter* highlighter_;

    QTimer *bgparseTimer_;
    const int bgparseInterval=1000;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    bool unsaved_;
    bool doBgParsing_;

    IQCADItemModel* cur_model_;
    
    IQISCADBackgroundThread bgparsethread_;
    
    bool skipPostprocActions_;

    QSize sizehint_;

    int fontSize_;
    bool hasBeenRebuilt_;

    void setFontSize(int fontSize);
    
protected:
    void clearDerivedData();

    inline void setFilename(const boost::filesystem::path& fn)
    {
        filename_=fn;
        emit updateTitle(filename_, false);
    }

    
public:
    IQISCADModelScriptEdit(QWidget* parent = 0, bool dobgparsing=true);
    ~IQISCADModelScriptEdit();
    
    void loadFile(const boost::filesystem::path& file);
    void setScript(const std::string& contents);
    
    /**
     * add all defined planes to the clip plane menu
     */
//    void populateClipPlaneMenu(QMenu* clipplanemenu, QoccViewWidget* v);

//    void connectModelTree(QModelTree* mt) const;
    void setModel(IQCADItemModel* model);

    inline bool isUnsaved() const { return unsaved_; }

    virtual QSize sizeHint() const;

public slots:

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
    void insertImportedModelAtCursor();
    void insertComponentNameAtCursor();
    void insertLibraryModelAtCursor();
    void insertDrawingAtCursor();

    void onIncreaseFontSize();
    void onDecreaseFontSize();
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

    void focus(insight::cad::FeaturePtr feat);
    void unfocus();

    void displayNeedsRefit();

//     // insert new features
//     void addFeature(const QString& sn, insight::cad::FeaturePtr sm, bool is_component);
//     void addDatum(const QString& sn, insight::cad::DatumPtr dm);
//     void addEvaluation(const QString& sn, insight::cad::PostprocActionPtr em, bool visible=false);
//     void addVariable(const QString& sn, insight::cad::parser::scalar sv);
//     void addVariable(const QString& sn, insight::cad::parser::vector vv);

};






#endif
