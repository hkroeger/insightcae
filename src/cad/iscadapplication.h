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
 *
 */

#ifndef ISCADAPPLICATION_H
#define ISCADAPPLICATION_H

#include <QApplication>
#include <QMainWindow>
#include <QMdiArea>
#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>

// #include "cadfeatures.h"

#include "qoccviewwidget.h"

#ifndef Q_MOC_RUN
#include "cadfeaturetransient.h"
#include "occinclude.h"
#include "cadfeature.h"
#include "parser.h"
#endif

class PointerTransient 
: public Standard_Transient
{
protected:
  QObject* mi_;
  
public:
    PointerTransient();
    PointerTransient(const PointerTransient& o);
    PointerTransient(QObject* mi);
    ~PointerTransient();
    void operator=(QObject* mi);
    QObject* getPointer();
    
};

struct ViewState
{
  int shading;
  bool visible;
  double r, g, b;
  
  ViewState();
  void randomizeColor();
};


class ISCADApplication
: public QApplication
{
  Q_OBJECT

public:
  ISCADApplication( int &argc, char **argv);
  ~ISCADApplication( );

  bool notify(QObject *rec, QEvent *ev);
};

class QoccViewerContext;
// class QoccViewWidget;

class QModelStepItem
: public QObject, public QListWidgetItem
{
  Q_OBJECT 
  
  QString name_;
  insight::cad::FeaturePtr smp_;
  QoccViewerContext* context_;
  Handle_AIS_Shape ais_;
    
signals:
  void jump_to(const QString& name);
  void insertParserStatementAtCursor(const QString& statement);
  void setUniformDisplayMode(const AIS_DisplayMode AM);

public:
  ViewState state_;

  QModelStepItem(const std::string& name, insight::cad::FeaturePtr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0);
  
  void reset(insight::cad::FeaturePtr smp);
  void wireframe();
  void shaded();
  void onlyThisShaded();
  void hide();
  void show();
  void randomizeColor();
  void updateDisplay();
  void exportShape();
  void insertName();
  
  inline insight::cad::Feature& solidmodel()
  {
    return *smp_;
  }
  
public slots:
  void showContextMenu(const QPoint& gpos);
};

class QVariableItem
: public QObject, public QListWidgetItem
{
  Q_OBJECT 
  
  QString name_;
  arma::mat value_;
  QoccViewerContext* context_;
  Handle_AIS_InteractiveObject ais_;
    
signals:
  void insertParserStatementAtCursor(const QString& statement);
 
protected:
  void createAISShape();
  
public:
  ViewState state_;

  QVariableItem(const std::string& name, arma::mat value, 
		QoccViewerContext* context, 
		const ViewState& state, QListWidget* view = 0);
  
  void reset(arma::mat value);
  void updateDisplay();
  void insertName();
  
public slots:
  void showContextMenu(const QPoint& gpos);
};


class ModelStepList
: public QListWidget
{
  Q_OBJECT
  
public:
  ModelStepList(QWidget* parent = 0);
  
protected slots:
  void showContextMenuForWidget(const QPoint &);
};

class DatumList
: public QListWidget
{
  Q_OBJECT
  
public:
  DatumList(QWidget* parent = 0);
  
protected slots:
  void showContextMenuForWidget(const QPoint &);
};

class EvaluationList
: public QListWidget
{
  Q_OBJECT
  
public:
  EvaluationList(QWidget* parent = 0);
  
protected slots:
  void showContextMenuForWidget(const QPoint &);
};




class ISCADHighlighter 
: public QSyntaxHighlighter
{
    Q_OBJECT

public:
    ISCADHighlighter(QTextDocument *parent = 0);

    void setHighlightWord(const QString& word);
    
protected:
    void highlightBlock(const QString &text);
    

private:
  enum HighlightingRule_Index
  {
    HighlightingRule_Function,
    HighlightingRule_ModelStepDef,
    HighlightingRule_CommentHash,
    HighlightingRule_SelectedKeyword,
    HighlightingRule_Index_Max
  };
    struct HighlightingRule
    {
	QRegExp pattern;
	QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
};



class ISCADMainWindow
: public QMainWindow
{
  Q_OBJECT

protected:
  boost::filesystem::path filename_;
  QoccViewerContext* context_;
  QoccViewWidget* viewer_;
  QListWidget* modelsteplist_;
  QListWidget* datumlist_;
  QListWidget* evaluationlist_;
  QListWidget* variablelist_;
  
  QTextEdit* editor_;
  ISCADHighlighter* highlighter_;
  
  std::map<std::string, ViewState> checked_modelsteps_, checked_datums_, checked_evaluations_;
  
  std::vector<Handle_AIS_InteractiveObject> additionalDisplayObjectsForSelection_;

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

public:
  ISCADMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  
  // insert model step
  void addModelStep(std::string sn, insight::cad::FeaturePtr sm, bool visible);
  void addDatum(std::string sn, insight::cad::DatumPtr dm);
  void addEvaluation(std::string sn, insight::cad::PostprocActionPtr em);
  void addVariable(std::string sn, insight::cad::parser::scalar sv);
  void addVariable(std::string sn, insight::cad::parser::vector vv);
  
  void loadFile(const boost::filesystem::path& file);

public slots:
  void loadModel();
  void saveModel();
  void saveModelAs();
  void rebuildModel();
  void popupMenu( QoccViewWidget* aView, const QPoint aPoint ); 
  

};

#endif
