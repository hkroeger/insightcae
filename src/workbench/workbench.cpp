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

#include "workbench.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>

#include "newanalysisdlg.h"
#include "analysisform.h"
#include "qinsighterror.h"

#include <fstream>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"


workbench::workbench()
{
    setWindowIcon(QIcon(":/resources/logo_insight_cae.png"));
    this->setWindowTitle("Insight Workbench");
    
    mdiArea_ = new QMdiArea(this);
    setCentralWidget( mdiArea_ );
    connect(mdiArea_, &QMdiArea::subWindowActivated,
            this, &workbench::onSubWindowActivated);
    
    QMenu *analysisMenu = menuBar()->addMenu( "&Analysis" );

    QAction* a = new QAction("New...", this); 
    a->setShortcut(Qt::ControlModifier + Qt::Key_N);
    connect(a, &QAction::triggered, this, &workbench::newAnalysis );
    analysisMenu->addAction( a );
    
    a = new QAction("Open...", this); 
    a->setShortcut(Qt::ControlModifier + Qt::Key_O);
    connect(a, &QAction::triggered, this, &workbench::onOpenAnalysis );
    analysisMenu->addAction( a );

    readSettings();
}

workbench::~workbench()
{}

void workbench::newAnalysis()
{
  newAnalysisDlg dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
    AnalysisForm *form;
    std::string analysisName = dlg.getAnalysisName();
    try
    {
      form = new AnalysisForm(mdiArea_, analysisName);
    }
    catch (const std::exception& e)
    {
      throw insight::Exception("Creation of an analysis of type \""+analysisName+"\" failed.\n"
                               "Reason: "+e.what());
    }
    form->showMaximized();
  }
}

void workbench::onOpenAnalysis()
{
  QString fn = QFileDialog::getOpenFileName(this, "Open Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty()) openAnalysis(fn);
}
    
void workbench::openAnalysis(const QString& fn)
{
  using namespace rapidxml;
  
  boost::filesystem::path fp(fn.toStdString());
  
  std::ifstream in(fp.c_str());
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();

  xml_document<> doc;
  doc.parse<0>(&contents[0]);
  
  xml_node<> *rootnode = doc.first_node("root");
  
  std::string analysisName;
  xml_node<> *analysisnamenode = rootnode->first_node("analysis");
  if (analysisnamenode)
  {
    analysisName = analysisnamenode->first_attribute("name")->value();
  }
  
  AnalysisForm *form;
  try
  {
    form = new AnalysisForm(mdiArea_, analysisName);
  }
  catch (const std::exception& e)
  {
    throw insight::Exception("Creation of an analysis of type \""+analysisName+"\" failed.\n"
                             "Please check, if the analysis type entry in the parameter file is correct.\n"
                             "Error information:\n"+e.what());
  }
  //form->parameters().readFromNode(doc, *rootnode, fp.parent_path());
  form->loadParameters(fp);
  boost::filesystem::path dir=boost::filesystem::path(fn.toStdString()).parent_path();
  form->executionPathParameter()()=dir;
  form->forceUpdate();
  form->showMaximized();
}


void workbench::closeEvent(QCloseEvent *event)
{
  QList<QMdiSubWindow*>	list = mdiArea_->subWindowList();

  for (int i = 0; i < list.size (); i++)
  {
    if (!list[i]->close ())
    {
      event->ignore();
      return;
    }
  }

  if (event->isAccepted())
  {
    QSettings settings("silentdynamics", "workbench_main");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
  }
}

void workbench::readSettings()
{
    QSettings settings("silentdynamics", "workbench_main");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}


void workbench::onSubWindowActivated( QMdiSubWindow * window )
{
    if (lastActive_)
    {
//        qDebug()<<"remove menu";
        lastActive_->removeMenu();
    }

    if (WidgetWithDynamicMenuEntries* newactive = dynamic_cast<WidgetWithDynamicMenuEntries*>(window))
    {
//        qDebug()<<"insert menu";
        newactive->insertMenu(menuBar());
        lastActive_=newactive;
    }
    else
    {
//        qDebug()<<"removed last menu";
        lastActive_=nullptr;
    }
}

//#include "workbench.moc"
