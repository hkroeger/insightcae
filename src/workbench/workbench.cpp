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

#include <fstream>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

WorkbenchApplication::WorkbenchApplication(int &argc, char **argv)
: QApplication(argc, argv)
{
  connect(this, &WorkbenchApplication::exceptionOcurred,
          this, &WorkbenchApplication::displayExceptionNotification);
}

WorkbenchApplication::~WorkbenchApplication()
{}

bool WorkbenchApplication::notify(QObject *rec, QEvent *ev)
{
  try
  {
    return QApplication::notify(rec, ev);
  }
  catch (insight::Exception e)
  {
    std::cout << e << std::endl;
    
    emit exceptionOcurred
    (
      QString(e.message().c_str()), 
      QString(e.strace().c_str())
    );
    
//     QMessageBox msgBox;
//     msgBox.setIcon(QMessageBox::Critical);
//     msgBox.setText(QString(e.as_string().c_str()));
// /*    if (e.addInfo()!="")
//     {
//       msgBox.setInformativeText("Please check additional info.");
//       msgBox.setDetailedText(QString(e.addInfo().c_str()));
//     }*/
//     msgBox.exec();
//    QMessageBox::critical
//    (
//        activeWindow(), "Error",
//        QString(("An error occured in PropGeo:\n"+e.message()).c_str())
//    );
  }
  /*
  catch (Standard_Failure e)
  {
    QMessageBox::critical
    (
        activeWindow(), "Error",
        QString("An error occured in OpenCASCADE:\n")+e.GetMessageString()
    );
  }*/

  return true;
}

void WorkbenchApplication::displayExceptionNotification(QString msg, QString addinfo)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(msg);
    if (addinfo!="")
    {
      msgBox.setInformativeText("See details for stack trace of error origin.");
      msgBox.setDetailedText(addinfo);
    }
    msgBox.exec();
}

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
    AnalysisForm *form= new AnalysisForm(mdiArea_, dlg.getAnalysisName());
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
  
  AnalysisForm *form= new AnalysisForm(mdiArea_, analysisName);
  //form->parameters().readFromNode(doc, *rootnode, fp.parent_path());
  form->loadParameters(fp);
  boost::filesystem::path dir=boost::filesystem::path(fn.toStdString()).parent_path();
  form->executionPathParameter()()=dir;
  form->forceUpdate();
  form->showMaximized();
}


void workbench::closeEvent(QCloseEvent *event)
{
    QSettings settings("silentdynamics", "workbench");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

void workbench::readSettings()
{
    QSettings settings("silentdynamics", "workbench");
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
        lastActive_=NULL;
    }
}

//#include "workbench.moc"
