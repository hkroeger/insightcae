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

#ifndef INSIGHT_CAD_ISCADMAINWINDOW_H
#define INSIGHT_CAD_ISCADMAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "qoccviewercontext.h"
#include "qoccviewwidget.h"
#include "qdebugstream.h"
#include "viewstate.h"

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


class BGParsingThread
: public QThread
{
    Q_OBJECT
    
protected:
    std::string script_;
    
    QStatusBar* statusbar_;
    
public:
    insight::cad::ModelPtr model_;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    
    BGParsingThread(QStatusBar*);
    
    void launch(const std::string& script);
    virtual void run();
};

class ISCADMainWindow
    : public QMainWindow
{
    Q_OBJECT

    friend class ModelStepItemAdder;

protected:
    boost::filesystem::path filename_;
    QoccViewerContext* context_;
    QoccViewWidget* viewer_;
    QListWidget* modelsteplist_;
    QListWidget* datumlist_;
    QListWidget* evaluationlist_;
    QListWidget* variablelist_;

    QTextEdit* editor_;
    ISCADSyntaxHighlighter* highlighter_;

    std::map<std::string, ViewState> checked_modelsteps_, checked_datums_, checked_evaluations_;

    std::vector<Handle_AIS_InteractiveObject> additionalDisplayObjectsForSelection_;

    Q_DebugStream* logger_;
    QTextEdit* log_;

    QTimer *bgparseTimer_;
    const int bgparseInterval=1000;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    bool unsaved_;
    bool doBgParsing_;
    
    insight::cad::ModelPtr cur_model_;
    
    BGParsingThread bgparsethread_;

protected:
    void clearDerivedData();
    virtual void closeEvent(QCloseEvent *event);

    inline void setFilename(const boost::filesystem::path& fn)
    {
        filename_=fn;
        setWindowTitle(filename_.filename().c_str());
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

protected slots:
    void onGraphicalSelectionChanged(QoccViewWidget* aView);
    void onVariableItemChanged(QListWidgetItem * item);
    void onModelStepItemChanged(QListWidgetItem * item);
    void onDatumItemChanged(QListWidgetItem * item);
    void onEvaluationItemChanged(QListWidgetItem * item);

    void onEditorSelectionChanged();

    void jump_to(const QString& name);

    void setUniformDisplayMode(const AIS_DisplayMode AM);

    void restartBgParseTimer(int i1=0,int i2=0,int i3=0);
    void doBgParse();

    void editSketch(int sk_ptr);
    void editModel(int mo_ptr);

    void toggleBgParsing(int state);
    
    void insertComponentNameAtCursor();
    
    void onBgParseFinished();

public:
    ISCADMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ISCADMainWindow();

    void loadFile(const boost::filesystem::path& file);

public slots:

    // insert model step
    void addModelStep(std::string sn, insight::cad::FeaturePtr sm, bool visible);
    void addDatum(std::string sn, insight::cad::DatumPtr dm);
    void addEvaluation(std::string sn, insight::cad::PostprocActionPtr em, bool visible=false);
    void addVariable(std::string sn, insight::cad::parser::scalar sv);
    void addVariable(std::string sn, insight::cad::parser::vector vv);

    void loadModel();
    void saveModel();
    void saveModelAs();
    void rebuildModel();
    void clearCache();
    void popupMenu( QoccViewWidget* aView, const QPoint aPoint );
    void showEditorContextMenu(const QPoint&);

    void setUnsavedState(int i1=0,int i2=0,int i3=0);
    void unsetUnsavedState();

};


#endif // INSIGHT_CAD_ISCADMAINWINDOW_H
