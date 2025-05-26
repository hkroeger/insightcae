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

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenuBar>
#include <QMenu>

#include "isofcasebuilderwindow.h"
#include "cadparametersetvisualizer.h"
#include "insertedcaseelement.h"
#include "iqvtkcadmodel3dviewer.h"

#ifndef Q_MOC_RUN
#include "openfoam/ofes.h"
#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/basic/setfieldsconfiguration.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh_templates.h"
#include "openfoam/snappyhexmesh.h"
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#endif

#include "base/qt5_helper.h"
#include "qtextensions.h"



using namespace insight;
using namespace boost;
using namespace rapidxml;




void isofCaseBuilderWindow::updateTitle()
{
  QString title="InsightCAE OpenFOAM Case Builder";
  if (!current_config_file_.empty())
  {
    title+=": "+QString::fromStdString(current_config_file_.string());
  }
  if (config_is_modified_)
  {
    title+="*";
  }
  this->setWindowTitle(title);
}




bool isofCaseBuilderWindow::CADisCollapsed() const
{
  QList<int> sz = ui->mainSplitter->sizes();
  return sz[0]==0 && sz[1]==0;
}




void isofCaseBuilderWindow::expandOrCollapseCADIfNeeded()
{
  if ( (multiVizSources_.size()>0) && CADisCollapsed())
  {
    // expand
    QList<int> sz = ui->mainSplitter->sizes();
    sz[0]=3*sz[2];
    sz[1]=sz[2];
    ui->mainSplitter->setSizes(sz);
  }
  else if ( (multiVizSources_.size()==0) && !CADisCollapsed())
  {
    QList<int> sz = ui->mainSplitter->sizes();
    sz[0]=0;
    sz[1]=0;
    ui->mainSplitter->setSizes(sz);
  }
}






isofCaseBuilderWindow::isofCaseBuilderWindow()
: QMainWindow(),
  script_pre_(""), script_mesh_(""), script_case_("")
{
  // setup layout
  ui = new Ui::isofCaseBuilderWindow;

  ui->setupUi(this);

  auto *cadview = new IQVTKCADModel3DViewer;
  ui->mainSplitter->insertWidget(0, cadview);

  display_=new IQVTKParameterSetDisplay(this, cadview, ui->modeltree);


  availableBCsModel_=new AvailableBCsModel(this);
  availableCaseElementsModel_=new AvailableCaseElementsModel(this);
  caseConfigModel_=new CaseConfigurationModel(this);

  BCConfigModel_=new BoundaryConfigurationModel(
        new DefaultPatch(multiVizSources_, this),
        this );
  ui->boundaryConfiguration->setAlternatingRowColors(true);
  ui->boundaryConfiguration->horizontalHeader()->setStretchLastSection(true);
  ui->boundaryConfiguration->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  auto *m = new QMenu("&File", this);
  menuBar()->addMenu(m);
  QAction *a;
  a = new QAction("Load configuration...", m);
  connect(a, &QAction::triggered,
          this, &isofCaseBuilderWindow::onLoad);
  m->addAction(a);
  a = new QAction("Save configuration", m);
  connect(a, &QAction::triggered,
          this, &isofCaseBuilderWindow::onSave);
  m->addAction(a);
  a = new QAction("Save configuration as...", m);
  connect(a, &QAction::triggered,
          this, &isofCaseBuilderWindow::onSaveAs);
  m->addAction(a);

  act_pack_=new QAction("Pack external files into config file", m);
  act_pack_->setCheckable(true);
  act_pack_->setChecked(pack_config_file_);
  connect(act_pack_, &QAction::triggered,
          this, &isofCaseBuilderWindow::onTogglePacked);
  m->addAction(act_pack_);

  connect(ui->btn_select_case_dir, &QPushButton::clicked,
          this, &isofCaseBuilderWindow::selectCaseDir);
  ui->case_dir->setText( QString::fromStdString(boost::filesystem::current_path().string()) );

  connect(ui->btn_paraview, &QPushButton::clicked,
          this, &isofCaseBuilderWindow::onStartPV);

  QMenu* startmenu=new QMenu(ui->btn_start);

  connect( ui->btn_start,
           &QPushButton::clicked,
           this, &isofCaseBuilderWindow::runAll);
  connect( startmenu->addAction(QIcon(":/symbole/run.svg"), "Execute everything (without cleaning)"),
           &QAction::triggered,
           this, &isofCaseBuilderWindow::runAll);

  connect( startmenu->addAction(QIcon(":/symbole/clean_and_run.svg"), "Clean and execute everything"),
           &QAction::triggered,
           this, &isofCaseBuilderWindow::cleanAndRunAll);

  connect( startmenu->addAction(QIcon(":/symbole/run_skip1.svg"), "Begin with mesh step"),
           &QAction::triggered,
           this, &isofCaseBuilderWindow::runMeshAndSolver);

  connect( startmenu->addAction(QIcon(":/symbole/run_skip2.svg"), "Begin with case step"),
           &QAction::triggered,
           this, &isofCaseBuilderWindow::runSolver);

  ui->btn_start->setMenu(startmenu);


  pe_layout_ = new QHBoxLayout(ui->parameter_editor);
  bc_pe_layout_ = new QHBoxLayout(ui->bc_parameter_editor);

  // populate list of available OF versions
  for (insight::OFEs::value_type ofe: insight::OFEs::list)
  {
    ui->OFversion->addItem(ofe.first.c_str());
  }

  std::string cofe = OFEs::currentOrPreferredOFE();
  if ( cofe != std::string() )
  {
    setOFVersion(cofe.c_str());
  }

  connect
      (
        ui->OFversion, QOverload<const QString&>::of(&QComboBox::currentIndexChanged),
        this, &isofCaseBuilderWindow::onOFVersionChanged
        );

  connect
      (
        ui->BCTab, &QTabWidget::currentChanged,
        this, &isofCaseBuilderWindow::onCurrentTabChanged
        );

  connect
      (
        ui->script_pre, &QPlainTextEdit::textChanged,
        this, &isofCaseBuilderWindow::onChange_script_pre
        );

  connect
      (
        ui->script_mesh, &QPlainTextEdit::textChanged,
        this, &isofCaseBuilderWindow::onChange_script_mesh
        );

  connect
      (
        ui->script_case, &QPlainTextEdit::textChanged,
        this, &isofCaseBuilderWindow::onChange_script_case
        );

  connect
      (
        ui->btn_reset_pre, &QPushButton::clicked,
        this, &isofCaseBuilderWindow::onReset_script_pre
        );
  connect
      (
        ui->btn_reset_mesh, &QPushButton::clicked,
        this, &isofCaseBuilderWindow::onReset_script_mesh
        );
  connect
      (
        ui->btn_reset_case, &QPushButton::clicked,
        this, &isofCaseBuilderWindow::onReset_script_case
        );



  /*
     *
     *
     *
     *
     * Case configuration actions
     *
     *
     *
     */

  ui->availableCaseElements->setModel(availableCaseElementsModel_);
  ui->caseConfiguration->setModel(caseConfigModel_);


  // case element selection changed, update parameter editor
  QObject::connect
      ( ui->caseConfiguration, &QListView::clicked,
        this, &isofCaseBuilderWindow::showParameterEditorForCaseElement );



  connect(ui->btnAddCaseElement, &QPushButton::clicked, this,
          [&]()
  {
    auto typeName = availableCaseElementsModel_->selectedCaseElementTypeName(
          ui->availableCaseElements->currentIndex() );

    if (typeName.empty())
    {
      QMessageBox::critical(this, "Invalid selection", "Please select a case element!");
      return;
    }

    auto *ice = new InsertedCaseElement(typeName, multiVizSources_, this);
    auto idx = caseConfigModel_->addCaseElement(ice);
    ui->caseConfiguration->setCurrentIndex(idx);
    showParameterEditorForCaseElement(idx);
  });


  connect(ui->btnRemoveCaseElement, &QPushButton::clicked, this,
          [&]()
          {
            auto cei = ui->caseConfiguration->currentIndex();
            if (cei.isValid())
            {
              auto answer=QMessageBox::question(
                    this, "Remove case element?",
                    "Remove the selected case element?\nNote: the parameter changes will be lost!",
                    QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
              if (answer==QMessageBox::Yes)
              {
                caseConfigModel_->removeElement(cei);
              }
            }
          }
          );




  /*
     *
     *
     * Patch related actions
     *
     *
     */

  ui->availableBCs->setModel(availableBCsModel_);
  ui->boundaryConfiguration->setModel(BCConfigModel_);


  QObject::connect
      ( ui->boundaryConfiguration, &QListView::clicked,
        this, &isofCaseBuilderWindow::showParameterEditorForPatch );

  connect(ui->btnParseBoundaryFile, &QPushButton::clicked, this,
          [&]()
  {
    insight::OFDictData::dict boundaryDict;

    ofc_->parseBoundaryDict(casepath(), boundaryDict);

    for (const auto& bde: boundaryDict)
    {
      BCConfigModel_->addUnconfiguredPatchIfNonexistent(bde.first);
    }
  });

  connect(ui->btnAddUnconfiguredPatch, &QPushButton::clicked, this,
          [&]()
  {
    QString pname = QInputDialog::getText(this, "Insert Patch", "Enter patch name:");
    if (!pname.isEmpty())
    {
      BCConfigModel_->addUnconfiguredPatchIfNonexistent(pname.toStdString());
    }
  });

  connect(ui->btnRemovePatch, &QPushButton::clicked, this,
          [&]()
  {
    auto ci = ui->boundaryConfiguration->currentIndex();
    if (ci.isValid())
    {
      auto answer=QMessageBox::question(
            this, "Remove Patch?",
            "Really remove the selected patch?\nNote: the parameters will be lost!",
            QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel );

      if (answer==QMessageBox::Yes)
      {
        BCConfigModel_->removePatch(ci);
      }
    }
  });

  connect(ui->btnRenamePatch, &QPushButton::clicked, this,
          [&]()
  {
    auto ci = ui->boundaryConfiguration->currentIndex();
    if (ci.isValid())
    {
      auto patch = BCConfigModel_->patch(ci);

      QString pname = QInputDialog::getText(
            this, "Rename Patch",
            "Enter new patch name:",
            QLineEdit::Normal,
            QString::fromStdString(patch->patch_name())
            );

      if (!pname.isEmpty())
      {
        BCConfigModel_->renamePatch(ci, pname);
      }
    }
  });

  connect(ui->btnClearPatchList, &QPushButton::clicked, this,
          [&]()
  {
    auto answer=QMessageBox::question(
          this, "Remove All Patches?",
          "Really remove all patches?\nNote: all boundary parameters will be lost!",
          QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel );

    if (answer==QMessageBox::Yes)
    {
      BCConfigModel_->clear();
    }
  });

  connect(ui->btnAssignBCType, &QPushButton::clicked, this,
          [&]()
  {
    auto typeName = availableBCsModel_->selectedBCType(
          ui->availableBCs->currentIndex() );
    if (typeName.empty())
    {
      QMessageBox::critical(this, "Invalid selection", "Please select a BC type!");
      return;
    }

    auto patchi = ui->boundaryConfiguration->currentIndex();
    if (!patchi.isValid())
    {
      QMessageBox::critical(this, "Invalid selection", "Please select a boundary patch!");
      return;
    }

    BCConfigModel_->resetBCType(patchi, typeName);

    showParameterEditorForPatch(patchi);
  });




  QMenu* createmenu=new QMenu(ui->create_btn);
  connect( createmenu->addAction("Create case and set up boundaries"), &QAction::triggered,
           this, &isofCaseBuilderWindow::onCreate);
  connect( createmenu->addAction("Skip boundary set up during case creation"), &QAction::triggered,
           this, &isofCaseBuilderWindow::onCreateNoBCs);
  ui->create_btn->setMenu(createmenu);


  connect(ui->create_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onCreate);
  connect(ui->clean_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onCleanCase);


  onOFVersionChanged(ui->OFversion->currentText());

  // global splitter
  ui->mainSplitter->setStretchFactor(0, 3);
  ui->mainSplitter->setStretchFactor(1, 1);
  ui->mainSplitter->setStretchFactor(2, 1);
  ui->mainSplitter->setStretchFactor(3, 0);
  ui->mainSplitter->setSizes({1000,300,300,50});


  // BC tab splitter
  ui->splitter_4->setStretchFactor(0, 1);
  ui->splitter_4->setStretchFactor(1, 1);

  setWindowIcon(QIcon(":/logo_insight_cae.png"));

  readSettings();

  updateTitle();
}



isofCaseBuilderWindow::~isofCaseBuilderWindow()
{
    // explicitly delete first
    delete caseElementParameterEditor_;
    delete patchParameterEditor_;
    delete caseConfigModel_;
    delete BCConfigModel_;
}


void isofCaseBuilderWindow::loadFile(const boost::filesystem::path& file, bool skipBCs)
{

    insight::XMLDocument doc(file);

    if (xml_node<> *OFEnode = doc.rootNode->first_node("OFE"))
    {
      std::string name = OFEnode->first_attribute("name")->value();
      ui->OFversion->setCurrentIndex(ui->OFversion->findText(name.c_str()));
      ui->saveOFversion->setChecked(true);
    }
    if (xml_node<> *script_node = doc.rootNode->first_node("script_pre"))
    {
      script_pre_=QString(script_node->first_attribute("code")->value());
    }
    if ( xml_node<> *script_node = doc.rootNode->first_node("script_mesh") )
    {
      script_mesh_=QString(script_node->first_attribute("code")->value());
    }
    if ( xml_node<> *script_node = doc.rootNode->first_node("script_case") )
    {
      script_case_=QString(script_node->first_attribute("code")->value());
    }


    caseConfigModel_->readFromNode(
          *doc.rootNode,
          multiVizSources_, this,
          file.parent_path() );

    if (!skipBCs)
    {
      auto *BCnode = doc.rootNode->first_node("BoundaryConditions");
      if (BCnode)
      {
        BCConfigModel_->readFromNode(
              *BCnode,
              multiVizSources_, this,
              file.parent_path() );
      }
    }

    current_config_file_=file;
    config_is_modified_=false;
    updateTitle();
}



void isofCaseBuilderWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("silentdynamics", "isofCaseBuilder");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("mainsplitter", ui->mainSplitter->saveState());
    settings.setValue("windowState_PE", ui->splitter_2->saveState());
    settings.setValue("windowState_BC_PE", ui->splitter_4->saveState());
    settings.setValue("PE_state", last_pe_state_);
    settings.setValue("BC_PE_state", last_bc_pe_state_);
    settings.setValue("pack_config_file", pack_config_file_);
    QMainWindow::closeEvent(event);
}

void isofCaseBuilderWindow::readSettings()
{
    QSettings settings("silentdynamics", "isofCaseBuilder");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->mainSplitter->restoreState(settings.value("mainsplitter").toByteArray());
    ui->splitter_2->restoreState(settings.value("windowState_PE").toByteArray());
    ui->splitter_4->restoreState(settings.value("windowState_BC_PE").toByteArray());
    last_pe_state_=settings.value("PE_state").toByteArray();
    last_bc_pe_state_=settings.value("BC_PE_state").toByteArray();

    pack_config_file_=settings.value("pack_config_file").toBool();
    act_pack_->setChecked(pack_config_file_);
}


void isofCaseBuilderWindow::onConfigModification()
{
  config_is_modified_=true;
  updateTitle();
}

void isofCaseBuilderWindow::onCurrentTabChanged(int idx)
{
  if (idx==2)
    onEnterRecipeTab();
}

void isofCaseBuilderWindow::onEnterRecipeTab()
{
  QPalette active_pal = ui->script_pre->palette();
  QPalette default_pal( active_pal );
  active_pal.setColor(QPalette::Text, Qt::black);
  default_pal.setColor(QPalette::Text, Qt::lightGray);

  if (script_pre_.isEmpty())
  {
    ui->script_pre->blockSignals(true);
    ui->script_pre->setPlainText( generateDefault_script_pre() );
    ui->script_pre->blockSignals(false);
    ui->script_pre->setPalette(default_pal);
  }
  else
  {
    ui->script_pre->blockSignals(true);
    ui->script_pre->setPlainText( script_pre_ );
    ui->script_pre->blockSignals(false);
    ui->script_pre->setPalette(active_pal);
  }
  if (script_mesh_.isEmpty())
  {
    ui->script_mesh->blockSignals(true);
    ui->script_mesh->setPlainText( generateDefault_script_mesh() );
    ui->script_mesh->blockSignals(false);
    ui->script_mesh->setPalette(default_pal);
  }
  else
  {
    ui->script_mesh->blockSignals(true);
    ui->script_mesh->setPlainText( script_mesh_ );
    ui->script_mesh->blockSignals(false);
    ui->script_mesh->setPalette(active_pal);
  }
  if (script_case_.isEmpty())
  {
    ui->script_case->blockSignals(true);
    ui->script_case->setPlainText( generateDefault_script_case() );
    ui->script_case->blockSignals(false);
    ui->script_case->setPalette(default_pal);
  }
  else
  {
    ui->script_case->blockSignals(true);
    ui->script_case->setPlainText( script_case_ );
    ui->script_case->blockSignals(false);
    ui->script_case->setPalette(active_pal);
  }
}

void isofCaseBuilderWindow::onChange_script_pre()
{
  QPalette active_pal = ui->script_pre->palette();
  active_pal.setColor(QPalette::Text, Qt::black);
  ui->script_pre->setPalette(active_pal);
  script_pre_ = ui->script_pre->toPlainText();
}

void isofCaseBuilderWindow::onChange_script_mesh()
{
  QPalette active_pal = ui->script_mesh->palette();
  active_pal.setColor(QPalette::Text, Qt::black);
  ui->script_mesh->setPalette(active_pal);
  script_mesh_ = ui->script_mesh->toPlainText();
}

void isofCaseBuilderWindow::onChange_script_case()
{
  QPalette active_pal = ui->script_case->palette();
  active_pal.setColor(QPalette::Text, Qt::black);
  ui->script_case->setPalette(active_pal);
  script_case_ = ui->script_case->toPlainText();
}

void isofCaseBuilderWindow::onReset_script_pre()
{
  script_pre_="";
  onEnterRecipeTab();
}

void isofCaseBuilderWindow::onReset_script_mesh()
{
  script_mesh_="";
  onEnterRecipeTab();
}

void isofCaseBuilderWindow::onReset_script_case()
{
  script_case_="";
  onEnterRecipeTab();
}




QString isofCaseBuilderWindow::generateDefault_script_pre()
{
  return QString();
}

QString isofCaseBuilderWindow::generateDefault_script_mesh()
{
  QString cmds;

  if (caseConfigModel_->containsCE<insight::bmd::blockMesh>(ui->OFversion->currentText()))
    cmds += "blockMesh\n";

  if (caseConfigModel_->containsCE<insight::snappyHexMeshConfiguration>(ui->OFversion->currentText()))
    cmds += "isofRun.py --mesh-reconst --reconst-only-latesttime --remove-processordirs  snappyHexMesh -overwrite\n";

  return cmds;
}

QString isofCaseBuilderWindow::generateDefault_script_case()
{
  QString cmds;

  if (caseConfigModel_->containsCE<insight::setFieldsConfiguration>(
        ui->OFversion->currentText()) )
    cmds += "isofRun.py --no-reconst setFields\n";

  QString app = caseConfigModel_->applicationName(
        ui->OFversion->currentText() );
  if ( ! (app.isEmpty() || app=="none") )
    cmds += "isofRun.py --reconst-only-latesttime " + app + "\n";

  return cmds;
}

boost::filesystem::path isofCaseBuilderWindow::casepath() const
{
  return boost::filesystem::path( ui->case_dir->text().toStdString() );
}


//void isofCaseBuilderWindow::onItemSelectionChanged()
//{
//    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->currentItem());
//    if (cur)
//    {
//        if (ped_)
//        {
//          last_pe_state_=ped_->saveState();
//          ped_->deleteLater();
//        }

////        insight::ParameterSet_VisualizerPtr viz;
//        insight::ParameterSet_ValidatorPtr vali;

////        try {
////            viz = insight::OpenFOAMCaseElement::visualizer(cur->type_name());
////        } catch (const std::exception& e)
////        { /* ignore, if non-existent */ }

//        try {
//            vali = insight::OpenFOAMCaseElement::validator(cur->type_name());
//        } catch (const std::exception& e)
//        { /* ignore, if non-existent */ }

//        ped_ = new ParameterEditorWidget
//               (
//                 cur->parameters(),
//                 cur->defaultParameters(),
//                 ui->parameter_editor,
//                 cur->visualizer(), vali,
//                 display_
//               );
//        connect(ped_, &ParameterEditorWidget::parameterSetChanged, ped_,
//                [&,cur]()
//                {
//                  cur->parameters() = ped_->model()->getParameterSet();
//                  onConfigModification();
//                }
//        );
//        pe_layout_->addWidget(ped_);

//        if (!last_pe_state_.isEmpty())
//        {
//          ped_->restoreState(last_pe_state_);
//        }
//    //     ui->parameter_editor->setCentralWidget(ped_);
        
//    //     ParameterSet emptyps;
//    //     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
//    }
//}



//void isofCaseBuilderWindow::onAddElement()
//{
//    QTreeWidgetItem* cur = ui->available_elements->currentItem();
//    if (cur && (cur->childCount()==0))
//    {
//        std::string type_name = cur->text(0).toStdString();
//        InsertedCaseElement* ice = new InsertedCaseElement(ui->selected_elements, type_name, display_);
//        if (ice->visualizer())
//          expandCAD();
//    }
//}


//void isofCaseBuilderWindow::onRemoveElement()
//{
//  {
//    InsertedCaseElement* cur
//        = dynamic_cast<InsertedCaseElement*> ( ui->selected_elements->currentItem() );
//    bool neededCAD(cur->visualizer());
//    if (cur)
//    {
//      delete cur;
//    }
//  }

//  // check whether CAD view is still needed
//  bool needsCAD=false;
//  for ( int i=0; i < ui->selected_elements->count(); i++ )
//  {
//    InsertedCaseElement* cur
//        = dynamic_cast<InsertedCaseElement*> ( ui->selected_elements->item ( i ) );
//    if ( cur )
//    {
//      needsCAD = needsCAD || cur->visualizer();
//    }
//  }



//  if (needsCAD && CADisCollapsed()) expandCAD();
//  if (!needsCAD) collapseCAD();

//}

//void isofCaseBuilderWindow::onMoveElementUp()
//{
//    QListWidgetItem* cur = ui->selected_elements->currentItem();
//    if (cur)
//    {
//        int r=ui->selected_elements->currentRow();
//        if (r>0)
//        {
//            QListWidgetItem* ci = ui->selected_elements->takeItem(r);
//            ui->selected_elements->insertItem(r - 1, ci);
//            ui->selected_elements->setCurrentRow(r-1);
//        }
//    }
//}

//void isofCaseBuilderWindow::onMoveElementDown()
//{
//    QListWidgetItem* cur = ui->selected_elements->currentItem();
//    if (cur)
//    {
//        int r=ui->selected_elements->currentRow();
//        if (r < ui->selected_elements->count())
//        {
//            QListWidgetItem* ci = ui->selected_elements->takeItem(r);
//            ui->selected_elements->insertItem(r + 1, ci);
//            ui->selected_elements->setCurrentRow(r+1);
//        }
//    }
//}




void isofCaseBuilderWindow::onSaveAs()
{

    QFileDialog fd(this);
    fd.setOption(QFileDialog::DontUseNativeDialog, true);
    fd.setWindowTitle("Save Parameters");
    QStringList filters;
    filters << "Insight Case Builder Parameter File (*.iscb)";
    fd.setNameFilters(filters);

    QCheckBox* cb = new QCheckBox;
    cb->setText("Pack: embed externally referenced files into configuration file");
    QGridLayout *fdl = static_cast<QGridLayout*>(fd.layout());
    int last_row=fdl->rowCount(); // id of new row below
    fdl->addWidget(cb, last_row, 0, 1, -1);

    cb->setChecked(pack_config_file_);

    if ( fd.exec() == QDialog::Accepted )
    {

      QString fn=fd.selectedFiles()[0];
      if (! (fn.endsWith(".iscb")||fn.endsWith(".ISCB")) )
        {
          fn+=".iscb";
        }

      current_config_file_=fn.toStdString();
      pack_config_file_=cb->isChecked();
      act_pack_->setChecked(pack_config_file_);
      onSave();
    }
}

void isofCaseBuilderWindow::onTogglePacked()
{
  pack_config_file_=act_pack_->isChecked();
}

void isofCaseBuilderWindow::onSave()
{

  if (current_config_file_.empty())
  {
    onSaveAs();
  }
  else
  {
    saveToFile(current_config_file_);

    config_is_modified_=false;
    updateTitle();

  }
}

void isofCaseBuilderWindow::saveToFile(const boost::filesystem::path& file)
{

    // == prepare XML document
    xml_document<> doc;

    // xml declaration
    xml_node<>* decl = doc.allocate_node ( node_declaration );
    decl->append_attribute ( doc.allocate_attribute ( "version", "1.0" ) );
    decl->append_attribute ( doc.allocate_attribute ( "encoding", "utf-8" ) );
    doc.append_node ( decl );

    xml_node<> *rootnode = doc.allocate_node ( node_element, "root" );
    doc.append_node ( rootnode );

    if (ui->saveOFversion->checkState()==Qt::Checked)
    {
      xml_node<> *OFEnode = doc.allocate_node ( node_element, "OFE" );
      OFEnode->append_attribute(
            doc.allocate_attribute(
              "name",
              doc.allocate_string(ui->OFversion->currentText().toStdString().c_str()))
            );
      rootnode->append_node ( OFEnode );
    }

    std::string code_pre, code_mesh, code_case; // store until XML file is written
    if (!script_pre_.isEmpty())
    {
      xml_node<> *script_node = doc.allocate_node ( node_element, "script_pre" );
      code_pre=script_pre_.toStdString();
      script_node->append_attribute(doc.allocate_attribute("code", code_pre.c_str()));
      rootnode->append_node ( script_node );
    }
    if (!script_mesh_.isEmpty())
    {
      xml_node<> *script_node = doc.allocate_node ( node_element, "script_mesh" );
      code_mesh=script_mesh_.toStdString();
      script_node->append_attribute(doc.allocate_attribute("code", code_mesh.c_str()));
      rootnode->append_node ( script_node );
    }
    if (!script_case_.isEmpty())
    {
      xml_node<> *script_node = doc.allocate_node ( node_element, "script_case" );
      code_case=script_case_.toStdString();
      script_node->append_attribute(doc.allocate_attribute("code", code_case.c_str()));
      rootnode->append_node ( script_node );
    }


    // == insert selected case elements
    caseConfigModel_->appendConfigurationToNode(doc, rootnode, pack_config_file_, file.parent_path());


    // insert configured patches
    xml_node<> *BCnode = doc.allocate_node ( node_element, "BoundaryConditions" );
    rootnode->append_node ( BCnode );
    BCConfigModel_->appendConfigurationToNode(doc, BCnode, pack_config_file_, file.parent_path());


    {
        std::ofstream f ( file.string() );
        f << doc;
        f << std::flush;
        f.close();
    }

}




void isofCaseBuilderWindow::onLoad()
{
    if (config_is_modified_)
    {
      auto res=QMessageBox::warning(this,
                           "Modifications",
                           "The configuration has been modified. "
                           "Changes will be lost after loading a new configuration. "
                           "Do you wish to save it first?",
                           QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
                 );
      if (res==QMessageBox::Yes)
      {
        onSave();
      }
      else if (res==QMessageBox::Cancel)
      {
        return;
      }
    }


    if (  auto fn = getFileName(
            this, "Load Parameters",
            GetFileMode::Open,
            {{"iscb", "Insight Case Builder Parameter File"}} ))
    {
        boost::filesystem::path file(fn);
        caseConfigModel_->clear();
        loadFile(file);
    }
}






void isofCaseBuilderWindow::showParameterEditorForCaseElement(const QModelIndex& index)
{
  if (index.isValid())
  {
    if (caseElementParameterEditor_)
    {
      last_pe_state_=caseElementParameterEditor_->saveState();
      caseElementParameterEditor_->deleteLater();
    }

    caseElementParameterEditor_ = caseConfigModel_->launchParameterEditor(
          index, ui->parameter_editor, display_
          );

    connect(caseElementParameterEditor_, &ParameterEditorWidget::parameterSetChanged,
            caseElementParameterEditor_,
            [&]() { onConfigModification(); } );

    pe_layout_->addWidget(caseElementParameterEditor_);

    if (!last_pe_state_.isEmpty())
    {
      caseElementParameterEditor_->restoreState(last_pe_state_);
    }
  }
}




void isofCaseBuilderWindow::showParameterEditorForPatch(const QModelIndex& index)
{
  if (index.isValid())
  {
    if (patchParameterEditor_)
    {
      last_bc_pe_state_=patchParameterEditor_->saveState();
      patchParameterEditor_->deleteLater();
    }

    patchParameterEditor_ = BCConfigModel_->launchParameterEditor(
          index, ui->bc_parameter_editor, display_
          );
    if (patchParameterEditor_)
    {

      connect(patchParameterEditor_, &ParameterEditorWidget::parameterSetChanged,
              patchParameterEditor_,
              [&]() { onConfigModification(); } );

      auto* patch = BCConfigModel_->patch(index);
      connect(patch, &QObject::destroyed,
              patch,
              [&]() { patchParameterEditor_->deleteLater(); } );

      bc_pe_layout_->addWidget(patchParameterEditor_);

      if (!last_bc_pe_state_.isEmpty())
      {
        patchParameterEditor_->restoreState(last_bc_pe_state_);
      }
    }
  }
}



void isofCaseBuilderWindow::onCleanCase()
{
  recreateOFCase(ui->OFversion->currentText());

  insight::OpenFOAMCaseDirs files(*ofc_, casepath());

  std::set<boost::filesystem::path> cands = files.caseFilesAndDirs();
  if (cands.size()>0)
  {
    QMessageBox msg;
    msg.addButton(QMessageBox::Yes);
    msg.addButton(QMessageBox::No);
    msg.addButton(QMessageBox::Cancel);
    msg.setText("The following files and directories will be deleted. Please confirm!");
    QString list;
    for (const auto & c: cands)
      list+=QString::fromStdString(c.filename().string())+"\n";
    msg.setDetailedText(list);
    if (msg.exec() == QMessageBox::Yes)
    {
      files.cleanCase();
    }
  }
  else
  {
    QMessageBox::information(this, "Nothing to clean", "No files or directories found, which could be removed during a clean operation.");
  }
}


bool isofCaseBuilderWindow::checkIfSaveNeeded()
{
  if (config_is_modified_)
  {
    auto res=QMessageBox::question(this,
                          "Config modifications",
                          "The configuration has not been saved. "
                          "Do you wish to save the configuration?",
                          QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
                          );
    if (res==QMessageBox::Yes)
    {
      onSave();
    }
    else if (res==QMessageBox::Cancel)
    {
      return false;
    }
  }

  return true;
}


void isofCaseBuilderWindow::onOFVersionChanged(const QString& ofename)
{
    recreateOFCase(ofename);
}


void isofCaseBuilderWindow::recreateOFCase(const QString& ofename)
{
  std::string ofen = ofename.toStdString();
//   std::cout<<ofen<<std::endl;
  ofc_.reset(new OpenFOAMCase(OFEs::get(ofen)));
}


Parameter& isofCaseBuilderWindow::caseElementParameter(int id, const std::string& path)
{
    return caseConfigModel_->caseElementParameterRef(id, path);
}




void isofCaseBuilderWindow::selectCaseDir()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Case Directory");
  if (!dir.isEmpty())
    ui->case_dir->setText(dir);
}


void isofCaseBuilderWindow::onStartPV()
{
  ::system( boost::str( boost::format
        ("cd %s; isPV.py &" ) % casepath().string()
   ).c_str() );
}

void isofCaseBuilderWindow::setOFVersion(const QString & ofename)
{
  ui->OFversion->setCurrentIndex(ui->OFversion->findText(ofename));
}

void isofCaseBuilderWindow::rebuildVisualization()
{
    // execute also, if number if visualizers is zero:
    // model will be cleared this way, after the last item is removed

    if (viz_ && !viz_->isFinished())
    {
        viz_->stopVisualizationComputation();
        viz_->deleteLater();
    }

    expandOrCollapseCADIfNeeded();

    viz_ = new insight::MultiCADParameterSetVisualizer(
        this,
        multiVizSources_,
        casepath(), consoleProgressDisplayer
        );

    // connect(
    //     viz_, &insight::CADParameterSetVisualizer::visualizationCalculationFinished, viz_,
    //     [this](bool success)
    //     { if (success) overlayText_->hide(); } );

    // connect(
    //     viz_, &insight::CADParameterSetVisualizer::visualizationComputationError, viz_,
    //     [this](insight::Exception ex)
    //     {
    //         overlayText_->setTextFormat(Qt::MarkdownText);
    //         overlayText_->setText(QString::fromStdString(
    //             "The visualization could not be generated.\n\n"
    //             "Reason:\n\n"
    //             "**"+ex.description()+"**\n\n"+
    //             boost::replace_all_copy(ex.context(), "\n", "\n\n")
    //             ));
    //         overlayText_->show();
    //     }
    //     );

    viz_->launch(display_->model());
}
