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
#include <QPushButton>
#include <QTabWidget>

#include "cadparametersetvisualizer.h"
#include "newanalysisdlg.h"
#include "analysisform.h"
#include "qinsighterror.h"
#include "iqremoteservereditdialog.h"
#include "iqconfigureexternalprogramsdialog.h"
#include "iqmanagereporttemplatesdialog.h"
#include "qanalysisthread.h"

#include <fstream>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

#include "base/toolkitversion.h"
#include "base/qt5_helper.h"
#include "base/translations.h"
#include "wslinstallation.h"
#include "newanalysisform.h"

WidgetWithDynamicMenuEntries::WidgetWithDynamicMenuEntries(
        QObject *parent,
        const std::vector<QObject *> &dynamicGUIElements )
    : QObject(parent),
      dynamicGUIElements_(dynamicGUIElements)
{
    connect( parent, &QObject::destroyed,
             this, &QObject::deleteLater );
}

WidgetWithDynamicMenuEntries::~WidgetWithDynamicMenuEntries()
{
    for (auto* e: dynamicGUIElements_)
    {
        delete e;
    }
}




void WorkbenchMainWindow::updateRecentFileActions()
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



AnalysisForm* WorkbenchMainWindow::addAnalysisTabWithDefaults(const std::string &analysisType)
{
    auto *form = new AnalysisForm(
        //mdiArea_,
        tw,
        analysisType,
        logToConsole_ );

    // form->showMaximized();

    int i=tw->addTab(form, QString::fromStdString(analysisType) );
    tw->setCurrentIndex(i);

    QString desc;
    QIcon icon(":analysis_default_icon.svg");
    if (insight::CADParameterSetModelVisualizer::iconForAnalysis_table().count(analysisType))
    {
        icon=insight::CADParameterSetModelVisualizer::iconForAnalysis(analysisType);
    }
    tw->setTabIcon(i, icon);

    return form;
}



WorkbenchMainWindow::WorkbenchMainWindow(bool logToConsole)
  : logToConsole_(logToConsole)
{
  setWindowIcon(QIcon(":/resources/logo_insight_cae.png"));
  this->setWindowTitle("InsightCAE Workbench");

  tw=new QTabWidget;
  tw->setTabsClosable(true);
  tw->setTabPosition(QTabWidget::TabPosition::West);
  setCentralWidget( tw );
  connect(tw, &QTabWidget::currentChanged, tw,
          [this](int i) { onAnalysisFormActivated(tw->widget(i)); } );
  connect(tw, &QTabWidget::tabCloseRequested,
          tw, [this](int i){ if (i>0) tw->removeTab(i); } );

  auto cw=new QWidget(this);
  auto vl=new QVBoxLayout;
  cw->setLayout(vl);

  auto availableAnalysesGallery_=new NewAnalysisForm;
  connect(availableAnalysesGallery_, &NewAnalysisForm::createAnalysis,
          this, &WorkbenchMainWindow::newAnalysis);
  connect(availableAnalysesGallery_, &NewAnalysisForm::openAnalysis,
          this, &WorkbenchMainWindow::onOpenAnalysis);
  vl->addWidget(availableAnalysesGallery_);
  tw->addTab(cw, "New Analysis");

  tw->tabBar()->tabButton(0, QTabBar::RightSide)->hide();

  // mdiArea_ = new SDMdiArea(this);
  // setCentralWidget( mdiArea_ );::
  // tw->addTab(mdiArea_, "Editor");
  // connect(mdiArea_, &QMdiArea::subWindowActivated,
  //         this, &WorkbenchMainWindow::onAnalysisFormActivated);

  QMenu *analysisMenu = menuBar()->addMenu( _("&Analysis") );

  QAction* a = new QAction( _("New..."), this);
  a->setShortcut(Qt::ControlModifier + Qt::Key_N);
  connect(a, &QAction::triggered,
          this, [&]() { newAnalysis(); } );
  analysisMenu->addAction( a );

  a = new QAction(_("Open..."), this);
  a->setShortcut(Qt::ControlModifier + Qt::Key_O);
  connect(a, &QAction::triggered, this, &WorkbenchMainWindow::onOpenAnalysis );
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

  QMenu *settingsMenu = menuBar()->addMenu( _("&Settings") );

  a = new QAction(_("Remote servers..."), this);
  connect(a, &QAction::triggered, this,
          [&]()
          {
            IQRemoteServerEditDialog dlg(this);
            dlg.exec();
          }
  );
  settingsMenu->addAction( a );

  a = new QAction(_("Configure paths to external programs..."), this);
  connect(a, &QAction::triggered, this,
          [&]()
          {
            IQConfigureExternalProgramsDialog dlg(this);
            dlg.exec();
          }
  );
  settingsMenu->addAction( a );


  a = new QAction(_("Manage report templates..."), this);
  connect(a, &QAction::triggered, this,
          [&]()
          {
            IQManageReportTemplatesDialog dlg(this);
            dlg.exec();
          }
  );
  settingsMenu->addAction( a );


  QMenu *helpMenu = menuBar()->addMenu( _("&Help") );

  QAction* ab = new QAction(_("About..."), this);
  helpMenu->addAction( ab );
  connect(ab, &QAction::triggered,
          [&]()
          {
            QMessageBox::information(
                  this,
          _("Workbench Information"),
            QString(_("InsightCAE Analysis Workbench\n"
                    "Version %1\n"))
              .arg(QString::fromStdString(insight::ToolkitVersion::current().toString()))
                  );
          }
  );



  tw->setCurrentIndex(0);

  readSettings();

#ifdef WIN32
    {
      QAction* be = new QAction(_("Check backend installation version..."), this);
        helpMenu->addAction( be );
        connect(be, &QAction::triggered,
                this, [&]() { checkInstallation(true); } );
    }
#endif
}




WorkbenchMainWindow::~WorkbenchMainWindow()
{}




void WorkbenchMainWindow::newAnalysis(std::string analysisType)
{
    if (analysisType.empty())
    {
        newAnalysisDlg dlg(this);
        dlg.exec();
        return;
    }

    try
    {
        addAnalysisTabWithDefaults(analysisType);
    }
    catch (const std::exception& e)
    {
        throw insight::Exception(_("Creation of an analysis of type \"%s\" failed.\n"
                                   "Reason: %s"), analysisType.c_str(), e.what());
    }
}




void WorkbenchMainWindow::onOpenAnalysis()
{
  QString fn = QFileDialog::getOpenFileName(
        this, _("Open Parameters"), QString(),
        _("Insight parameter sets (*.ist)") );
  if (!fn.isEmpty()) openAnalysis(fn);
}




void WorkbenchMainWindow::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
    openAnalysis(action->data().toString());
}





void WorkbenchMainWindow::checkInstallation(bool reportSummary)
{
    insight::checkExternalPrograms(this);
#ifdef WIN32
    insight::checkWSLVersions(reportSummary, this);
#endif
}




void WorkbenchMainWindow::openAnalysis(const QString& fn)
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
  
  AnalysisForm *form = nullptr;
  try
  {
      form = addAnalysisTabWithDefaults( analysisName );
  }
  catch (const std::exception& e)
  {
    throw insight::Exception(
        _("Creation of an analysis of type \"%s\" failed.\n"
          "Please check, if the analysis type entry in the parameter file is correct.\n"
          "Error information: %s\n"), analysisName.c_str(), e.what());
  }
  //form->parameters().readFromNode(doc, *rootnode, fp.parent_path());
  form->loadParameters(fp);
  Q_EMIT update();
  form->showMaximized();
}


void WorkbenchMainWindow::closeEvent(QCloseEvent *event)
{
  // QList<QMdiSubWindow*>	list = mdiArea_->subWindowList();

  QList<QWidget*>	list;
  for (int i=0; i<tw->count(); ++i)
        list.append(tw->widget(i));

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

void WorkbenchMainWindow::readSettings()
{
    QSettings settings("silentdynamics", "workbench_main");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void WorkbenchMainWindow::show()
{
    QMainWindow::show();
#ifdef WIN32
    checkInstallation(false);
#endif
}


void WorkbenchMainWindow::onAnalysisFormActivated( QWidget * widget )
{
    if (lastActive_)
    {
        delete lastActive_;
    }

    if (auto* newactive = dynamic_cast<AnalysisForm*>(widget))
    {
        lastActive_=newactive->createMenus(menuBar());
    }
    else
    {
        lastActive_=nullptr;
    }
}

//#include "workbench.moc"
