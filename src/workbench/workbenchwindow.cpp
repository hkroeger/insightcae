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

#include "workbenchwindow.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QSettings>

#include "newanalysisdlg.h"
#include "analysisform.h"
#include "qinsighterror.h"
#include "iqremoteservereditdialog.h"
#include "iqconfigureexternalprogramsdialog.h"
#include "qanalysisthread.h"

#include <fstream>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

#include "base/toolkitversion.h"
#include "base/qt5_helper.h"
#include "wslinstallation.h"

void workbench::updateRecentFileActions()
{
  QSettings settings;
  QStringList files;
  if (settings.contains("recentFileList"))
    files = settings.value("recentFileList").toStringList();

  int numRecentFiles = std::min<int>(files.size(), recentFileActs_.size());

  for (int i = 0; i < numRecentFiles; ++i)
  {
      QString text = tr("&%1 %2")
          .arg(i + 1)
          .arg( QFileInfo(files[i]).fileName() )
          ;
      recentFileActs_[i]->setText(text);
      recentFileActs_[i]->setData(files[i]);
      recentFileActs_[i]->setVisible(true);
  }
  for (size_t j = numRecentFiles; j < recentFileActs_.size(); ++j)
      recentFileActs_[j]->setVisible(false);

  separatorAct_->setVisible(numRecentFiles > 0);
}




workbench::workbench(bool logToConsole)
  : logToConsole_(logToConsole)
{
  setWindowIcon(QIcon(":/resources/logo_insight_cae.png"));
  this->setWindowTitle("InsightCAE Workbench");

  mdiArea_ = new SDMdiArea(this);
  setCentralWidget( mdiArea_ );
  connect(mdiArea_, &QMdiArea::subWindowActivated,
          this, &workbench::onSubWindowActivated);

  QMenu *analysisMenu = menuBar()->addMenu( "&Analysis" );

  QAction* a = new QAction("New...", this);
  a->setShortcut(Qt::ControlModifier + Qt::Key_N);
  connect(a, &QAction::triggered,
          this, [&]() { newAnalysis(); } );
  analysisMenu->addAction( a );

  a = new QAction("Open...", this);
  a->setShortcut(Qt::ControlModifier + Qt::Key_O);
  connect(a, &QAction::triggered, this, &workbench::onOpenAnalysis );
  analysisMenu->addAction( a );

  separatorAct_ = analysisMenu->addSeparator();
  for (size_t i = 0; i < recentFileActs_.size(); ++i)
  {
      recentFileActs_[i] = new QAction(this);

      recentFileActs_[i]->setVisible(false);
      analysisMenu->addAction(recentFileActs_[i]);

      connect(recentFileActs_[i], SIGNAL(triggered()),
              this, SLOT(openRecentFile()));
  }
  updateRecentFileActions();

  QMenu *settingsMenu = menuBar()->addMenu( "&Settings" );

  a = new QAction("Remote servers...", this);
  connect(a, &QAction::triggered, this,
          [&]()
          {
            IQRemoteServerEditDialog dlg(this);
            dlg.exec();
          }
  );
  settingsMenu->addAction( a );

  a = new QAction("Configure paths to external programs...", this);
  connect(a, &QAction::triggered, this,
          [&]()
          {
            IQConfigureExternalProgramsDialog dlg(this);
            dlg.exec();
          }
  );
  settingsMenu->addAction( a );


  QMenu *helpMenu = menuBar()->addMenu( "&Help" );

  QAction* ab = new QAction("About...", this);
  helpMenu->addAction( ab );
  connect(ab, &QAction::triggered,
          [&]()
          {
            QMessageBox::information(
                  this,
                  "Workbench Information",
                  "InsightCAE Analysis Workbench\n"
                  "Version "+QString::fromStdString(insight::ToolkitVersion::current().toString())+"\n"
                  );
          }
  );

  readSettings();

#ifdef WIN32
  {
    checkInstallation(false);
    QAction* be = new QAction("Check backend installation version...", this);
    helpMenu->addAction( be );
    connect(be, &QAction::triggered,
            this, [&]() { checkInstallation(true); } );
  }
#endif
}




workbench::~workbench()
{}




void workbench::newAnalysis(std::string analysisType)
{
    if (analysisType.empty())
    {
        newAnalysisDlg dlg(this);
        if (dlg.exec() == QDialog::Accepted)
        {
            analysisType = dlg.getAnalysisName();
        }
    }

    try
    {
        AnalysisForm *form = new AnalysisForm(mdiArea_, analysisType, logToConsole_);
        form->showMaximized();
    }
    catch (const std::exception& e)
    {
        throw insight::Exception("Creation of an analysis of type \""+analysisType+"\" failed.\n"                                                                        "Reason: "+e.what());
    }
}




void workbench::onOpenAnalysis()
{
  QString fn = QFileDialog::getOpenFileName(this, "Open Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty()) openAnalysis(fn);
}




void workbench::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
    openAnalysis(action->data().toString());
}





void workbench::checkInstallation(bool reportSummary)
{
    insight::checkExternalPrograms(this);
#ifdef WIN32
    insight::checkWSLVersions(reportSummary, this);
#endif
}




void workbench::openAnalysis(const QString& fn)
{

  QSettings settings;
  QStringList files = settings.value("recentFileList").toStringList();
  files.removeAll(fn);
  files.prepend(fn);
  while (files.size() > recentFileActs_.size())
      files.removeLast();
  settings.setValue("recentFileList", files);
  updateRecentFileActions();

  using namespace rapidxml;
  
  boost::filesystem::path fp(fn.toStdString());
  
  std::string contents;
  insight::readFileIntoString(fp, contents);

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
    form = new AnalysisForm(mdiArea_,
                            analysisName,
                            logToConsole_
                            );
  }
  catch (const std::exception& e)
  {
    throw insight::Exception("Creation of an analysis of type \""+analysisName+"\" failed.\n"
                             "Please check, if the analysis type entry in the parameter file is correct.\n"
                             "Error information:\n"+e.what());
  }
  //form->parameters().readFromNode(doc, *rootnode, fp.parent_path());
  form->loadParameters(fp);
  Q_EMIT update();
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
