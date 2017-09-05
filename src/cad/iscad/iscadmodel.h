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




class BGParsingThread
: public QThread
{
    Q_OBJECT
    
protected:
    std::string script_;
    
public:
    insight::cad::ModelPtr model_;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    
    BGParsingThread();
    
    void launch(const std::string& script);
    virtual void run();
};






class ISCADModel
: public QWidget
{
    Q_OBJECT
    
    friend class ISCADMainWindow;
    
protected:
    boost::filesystem::path filename_;
    QoccViewerContext* context_;
    QoccViewWidget* viewer_;
    QModelTree* modeltree_;

    QTextEdit* editor_;
    ISCADSyntaxHighlighter* highlighter_;

    std::map<std::string, ViewState> checked_modelsteps_, checked_datums_, checked_evaluations_;

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
                    if (PointerTransient *smo=dynamic_cast<PointerTransient*>(own.Access()))
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
    ISCADModel(QWidget* parent = 0);
    ~ISCADModel();
    
    void loadFile(const boost::filesystem::path& file);
    void setScript(const std::string& contents);
    void populateClipPlaneMenu(QMenu* clipplanemenu);

protected slots:
    void onGraphicalSelectionChanged(QoccViewWidget* aView);
    void onModelTreeItemChanged(QTreeWidgetItem * item, int);

    void onEditorSelectionChanged();

    void jump_to(const QString& name);

    void restartBgParseTimer(int i1=0,int i2=0,int i3=0);
    void doBgParse();

    void editSketch(QObject* sk_ptr);
    void editModel(QObject* mo_ptr);

    void toggleBgParsing(int state);
    void toggleSkipPostprocActions(int state);
    
    void insertSectionCommentAtCursor();
    void insertFeatureAtCursor();
    void insertComponentNameAtCursor();
    
    void onBgParseFinished();
    
    void onCopyBtnClicked();
    
    void allShaded();
    void allWireframe();
    
    void onSetClipPlane(QObject* datumplane);
    
public slots:

    // insert model step
    void addFeature(std::string sn, insight::cad::FeaturePtr sm, bool is_component);
    void addDatum(std::string sn, insight::cad::DatumPtr dm);
    void addEvaluation(std::string sn, insight::cad::PostprocActionPtr em, bool visible=false);
    void addVariable(std::string sn, insight::cad::parser::scalar sv);
    void addVariable(std::string sn, insight::cad::parser::vector vv);

    bool saveModel();
    bool saveModelAs();
    void rebuildModel(bool upToCursor=false);
    void rebuildModelUpToCursor();
    void clearCache();
    void popupMenu( QoccViewWidget* aView, const QPoint aPoint );
    void showEditorContextMenu(const QPoint&);

    void setUnsavedState(int i1=0, int i2=1, int i3=1);
    void unsetUnsavedState();
    
signals:
    void displayStatus(const QString&);
    void updateTabTitle(ISCADModel* model, const boost::filesystem::path& filepath, bool isUnSaved);
    void updateClipPlaneMenu();
    void openModel(const boost::filesystem::path& modelfile);

};


#endif
