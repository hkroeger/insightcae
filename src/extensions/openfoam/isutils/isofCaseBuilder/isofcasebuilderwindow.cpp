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
#include "insertedcaseelement.h"

#ifndef Q_MOC_RUN
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh_templates.h"
#include "openfoam/snappyhexmesh.h"
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#endif

#include "base/qt5_helper.h"

using namespace insight;
using namespace boost;
using namespace rapidxml;


class HierarchyLevel
: public std::map<std::string, HierarchyLevel>
{
public:
    QTreeWidgetItem* parent_;
    
    HierarchyLevel(QTreeWidgetItem* parent)
    : parent_(parent)
    {}
    
    iterator addHierarchyLevel(const std::string& entry)
    {
        QTreeWidgetItem* newnode = new QTreeWidgetItem(parent_, QStringList() << entry.c_str());
        { QFont f=newnode->font(0); f.setBold(true); newnode->setFont(0, f); }
        std::pair<iterator,bool> ret = insert(std::make_pair(entry, HierarchyLevel(newnode)));
        return ret.first;
    }
    
    HierarchyLevel& sublevel(const std::string& entry)
    {
        iterator it = find(entry);
        if (it == end())
        {
            it=addHierarchyLevel(entry);
        }
        return it->second;
    }
};




void isofCaseBuilderWindow::fillCaseElementList()
{
  QTreeWidgetItem *topitem = new QTreeWidgetItem ( ui->available_elements, QStringList() << "Available Case Elements" );
  { QFont f=topitem->font(0); f.setBold(true); f.setPointSize(f.pointSize()+1); topitem->setFont(0, f); }
  HierarchyLevel toplevel ( topitem );
  
  /*HierarchyLevel::iterator i=*/toplevel.addHierarchyLevel("Uncategorized");

  for ( 
      insight::OpenFOAMCaseElement::FactoryTable::const_iterator i =
         insight::OpenFOAMCaseElement::factories_->begin();
      i != insight::OpenFOAMCaseElement::factories_->end(); 
      i++ 
    )
    {
      std::string elemName = i->first;
      QStringList path = QString::fromStdString 
        ( 
            insight::OpenFOAMCaseElement::category ( elemName ) 
        ).split ( "/", QString::SkipEmptyParts );
      HierarchyLevel* parent = &toplevel;
      for ( QStringList::const_iterator pit = path.constBegin(); pit != path.constEnd(); ++pit )
        {
          parent = & ( parent->sublevel ( pit->toStdString() ) );
        }
      /*QTreeWidgetItem* item =*/ new QTreeWidgetItem ( parent->parent_, QStringList() << elemName.c_str() );
//       QFont f=item->font(0); f.setBold(true); item->setFont(0, f);
    }
    
  ui->available_elements->expandItem(topitem);
}


isofCaseBuilderWindow::isofCaseBuilderWindow()
: QMainWindow(), ped_(nullptr), bc_ped_(nullptr),
  script_pre_(""), script_mesh_(""), script_case_("")
{
    // setup layout
    ui = new Ui::isofCaseBuilderWindow;
    ui->setupUi(this);

    ui->occview->connectModelTree(ui->modeltree);

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

    connect(ui->btn_select_case_dir, &QPushButton::clicked,
            this, &isofCaseBuilderWindow::selectCaseDir);
    ui->case_dir->setText( boost::filesystem::current_path().c_str() );

    QMenu* startmenu=new QMenu(ui->btn_start);

    connect( ui->btn_start,
             &QPushButton::clicked,
             this, &isofCaseBuilderWindow::runAll);
    connect( startmenu->addAction("Execute everything (without cleaning)"),
             &QAction::triggered,
             this, &isofCaseBuilderWindow::runAll);

    connect( startmenu->addAction("Clean and execute everything"),
             &QAction::triggered,
             this, &isofCaseBuilderWindow::cleanAndRunAll);

    connect( startmenu->addAction("Begin with mesh step"),
             &QAction::triggered,
             this, &isofCaseBuilderWindow::runMeshAndSolver);

    connect( startmenu->addAction("Begin with mesh case step"),
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
    
    std::string cofe=OFEs::detectCurrentOFE();
    if ( cofe != std::string() )
    {
      ui->OFversion->setCurrentIndex(ui->OFversion->findText(cofe.c_str()));
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

    fillCaseElementList();
//     // populate list of available case elements
//     for (insight::OpenFOAMCaseElement::FactoryTable::const_iterator i = insight::OpenFOAMCaseElement::factories_->begin();
//         i != insight::OpenFOAMCaseElement::factories_->end(); i++)
//     {
//         new QListWidgetItem(i->first.c_str(), ui->available_elements);
//     }
    
    // populate list of available boundary condition elements
    for (insight::BoundaryCondition::FactoryTable::const_iterator i = insight::BoundaryCondition::factories_->begin();
        i != insight::BoundaryCondition::factories_->end(); i++)
    {
        new QListWidgetItem(i->first.c_str(), ui->bc_element_list);
    }
    
    QObject::connect 
    ( 
        ui->selected_elements, &QListWidget::itemSelectionChanged,
        this, &isofCaseBuilderWindow::onItemSelectionChanged
    );
    
    connect(ui->add_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onAddElement);
    connect(ui->remove_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onRemoveElement);
    connect(ui->up_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onMoveElementUp);
    connect(ui->down_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onMoveElementDown);

    QMenu* createmenu=new QMenu(ui->create_btn);
    connect( createmenu->addAction("Create case and set up boundaries"), &QAction::triggered,
             this, &isofCaseBuilderWindow::onCreate);
    connect( createmenu->addAction("Skip boundary set up during case creation"), &QAction::triggered,
             this, &isofCaseBuilderWindow::onCreateNoBCs);
    ui->create_btn->setMenu(createmenu);
    connect(ui->create_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onCreate);

    connect(ui->clean_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onCleanCase);
//    connect(ui->load_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onLoad);

    connect(ui->parse_bf_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onParseBF);
    connect(ui->add_patch_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onAddPatchManually);
    connect(ui->assign_bc_btn, &QPushButton::clicked, this, &isofCaseBuilderWindow::onAssignBC);
    
    QObject::connect 
    ( 
        ui->patch_list, &QListWidget::itemSelectionChanged,
        this, &isofCaseBuilderWindow::onPatchSelectionChanged
    );
    
    onOFVersionChanged(ui->OFversion->currentText());

    // global splitter
    ui->splitter_5->setStretchFactor(0, 3);
    ui->splitter_5->setStretchFactor(1, 0);
    ui->splitter_5->setStretchFactor(2, 1);

    collapseCAD();

//    // case element splitter
//    ui->splitter_2->setStretchFactor(0, 1);
//    ui->splitter_2->setStretchFactor(1, 1);

    // BC tab splitter
    ui->splitter_4->setStretchFactor(0, 1);
    ui->splitter_4->setStretchFactor(1, 1);

    setWindowIcon(QIcon(":/logo_insight_cae.png"));

    readSettings();

    updateTitle();
}




isofCaseBuilderWindow::~isofCaseBuilderWindow()
{
}


void isofCaseBuilderWindow::loadFile(const boost::filesystem::path& file, bool skipBCs)
{
    std::ifstream in(file.c_str());
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    xml_document<> doc;
    doc.parse<0>(&contents[0]);
    
    xml_node<> *rootnode = doc.first_node("root");

    if (xml_node<> *OFEnode = rootnode->first_node("OFE"))
    {
      std::string name = OFEnode->first_attribute("name")->value();
      ui->OFversion->setCurrentIndex(ui->OFversion->findText(name.c_str()));
    }
    if (xml_node<> *script_node = rootnode->first_node("script_pre"))
    {
      script_pre_=QString(script_node->first_attribute("code")->value());
    }
    if ( xml_node<> *script_node = rootnode->first_node("script_mesh") )
    {
      script_mesh_=QString(script_node->first_attribute("code")->value());
    }
    if ( xml_node<> *script_node = rootnode->first_node("script_case") )
    {
      script_case_=QString(script_node->first_attribute("code")->value());
    }

    bool needsCAD=false;
    for (xml_node<> *e = rootnode->first_node("OpenFOAMCaseElement"); e; e = e->next_sibling("OpenFOAMCaseElement"))
    {
        std::string type_name = e->first_attribute("type")->value();
    
        InsertedCaseElement* ice = new InsertedCaseElement(ui->selected_elements, type_name);
        ice->parameters().readFromNode(doc, *e, file.parent_path());
        needsCAD = needsCAD || ice->hasVisualization();
    }

    if (!skipBCs)
    {
      xml_node<> *BCnode = rootnode->first_node("BoundaryConditions");
      if (BCnode)
      {
	      xml_node<> *unassignedBCnode = BCnode->first_node( "UnassignedPatches" );
	      new DefaultPatch(ui->patch_list, doc, *unassignedBCnode, file.parent_path());
	      
	      for (xml_node<> *e = BCnode->first_node("Patch"); e; e = e->next_sibling("Patch"))
	      {
		    new Patch(ui->patch_list, doc, *e, file.parent_path());
	      }
      }
    }

    if (needsCAD && CADisCollapsed()) expandCAD();
    if (!needsCAD) collapseCAD();

    current_config_file_=file;
    config_is_modified_=false;
    updateTitle();
}



void isofCaseBuilderWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("silentdynamics", "isofCaseBuilder");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("windowState_PE", ui->splitter_2->saveState());
    settings.setValue("windowState_BC_PE", ui->splitter_4->saveState());
    settings.setValue("PE_state", last_pe_state_);
    settings.setValue("BC_PE_state", last_bc_pe_state_);
    QMainWindow::closeEvent(event);
}

void isofCaseBuilderWindow::readSettings()
{
    QSettings settings("silentdynamics", "isofCaseBuilder");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->splitter_2->restoreState(settings.value("windowState_PE").toByteArray());
    ui->splitter_4->restoreState(settings.value("windowState_BC_PE").toByteArray());
    last_pe_state_=settings.value("PE_state").toByteArray();
    last_bc_pe_state_=settings.value("BC_PE_state").toByteArray();
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

void isofCaseBuilderWindow::expandCAD()
{
  QList<int> sz = ui->splitter_5->sizes();
  sz[2]=300;
  sz[0]=3*sz[2];
  sz[1]=sz[2];
  ui->splitter_5->setSizes(sz);
}


void isofCaseBuilderWindow::collapseCAD()
{
  QList<int> sz = ui->splitter_5->sizes();
  sz[0]=0;
  sz[1]=0;
  sz[2]=600;
  ui->splitter_5->setSizes(sz);
}


QString isofCaseBuilderWindow::applicationName() const
{
  insight::OpenFOAMCase ofc(insight::OFEs::get(ui->OFversion->currentText().toStdString()));
  for ( int i=0; i < ui->selected_elements->count(); i++ )
    {
      InsertedCaseElement* cur
        = dynamic_cast<InsertedCaseElement*> ( ui->selected_elements->item ( i ) );
      if ( cur )
        {
          std::auto_ptr<insight::OpenFOAMCaseElement> ce( cur->createElement(ofc) );
          if ( const auto* fvn = dynamic_cast<const insight::FVNumerics*>(ce.get()) )
          {
            insight::OFdicts dicts;
            fvn->addIntoDictionaries(dicts);
            try
            {
              OFDictData::dict cd = dicts.lookupDict("system/controlDict");
              std::string appname = cd.getString("application");
              return QString(appname.c_str());
            }
            catch (insight::Exception e)
            {
              // continue
            }
          }
        }
    }
  return QString();
}

QString isofCaseBuilderWindow::generateDefault_script_pre()
{
  return QString();
}

QString isofCaseBuilderWindow::generateDefault_script_mesh()
{
  QString cmds;

  if (containsCE<insight::bmd::blockMesh>())
    cmds += "blockMesh\n";

  if (containsCE<insight::snappyHexMeshConfiguration>())
    cmds += "isofRun.py --mesh-reconst --reconst-only-latesttime  snappyHexMesh\n";

  return cmds;
}

QString isofCaseBuilderWindow::generateDefault_script_case()
{
  QString cmds;

  if (containsCE<insight::setFieldsConfiguration>())
    cmds += "isofRun.py --no-reconst setFields\n";

  QString app = applicationName();
  if ( ! (app.isEmpty() || app=="none") )
    cmds += "isofRun.py --reconst-only-latesttime " + app + "\n";

  return cmds;
}

boost::filesystem::path isofCaseBuilderWindow::casepath() const
{
  return boost::filesystem::path( ui->case_dir->text().toStdString() );
}


void isofCaseBuilderWindow::onItemSelectionChanged()
{
    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->currentItem());
    if (cur)
    {
        if (ped_)
        {
          last_pe_state_=ped_->saveState();
          ped_->deleteLater();
        }

        insight::ParameterSet_VisualizerPtr viz;
        insight::ParameterSet_ValidatorPtr vali;

        try {
            viz = insight::OpenFOAMCaseElement::visualizer(cur->type_name());
        } catch (insight::Exception e)
        { /* ignore, if non-existent */ }

        try {
            vali = insight::OpenFOAMCaseElement::validator(cur->type_name());
        } catch (insight::Exception e)
        { /* ignore, if non-existent */ }

        ped_ = new ParameterEditorWidget(cur->parameters(), ui->parameter_editor,
                                         vali, viz,
                                         ui->occview, ui->modeltree);
        connect(ped_, &ParameterEditorWidget::parameterSetChanged,
                this, &isofCaseBuilderWindow::onConfigModification);
        pe_layout_->addWidget(ped_);

        if (!last_pe_state_.isEmpty())
        {
          ped_->restoreState(last_pe_state_);
        }
    //     ui->parameter_editor->setCentralWidget(ped_);
        
    //     ParameterSet emptyps;
    //     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
    }
}



void isofCaseBuilderWindow::onAddElement()
{
    QTreeWidgetItem* cur = ui->available_elements->currentItem();
    if (cur && (cur->childCount()==0))
    {
        std::string type_name = cur->text(0).toStdString();
        InsertedCaseElement* ice = new InsertedCaseElement(ui->selected_elements, type_name);
        if (ice->hasVisualization())
          expandCAD();
    }
}


void isofCaseBuilderWindow::onRemoveElement()
{
    QListWidgetItem* cur = ui->selected_elements->currentItem();
    if (cur)
    {
        delete cur;
    }

    // check whether CAD view is still needed
    bool needsCAD=false;
    for ( int i=0; i < ui->selected_elements->count(); i++ )
      {
        InsertedCaseElement* cur
          = dynamic_cast<InsertedCaseElement*> ( ui->selected_elements->item ( i ) );
        if ( cur )
          {
            needsCAD = needsCAD || cur->hasVisualization();
          }
      }
    if (needsCAD && CADisCollapsed()) expandCAD();
    if (!needsCAD) collapseCAD();

}

void isofCaseBuilderWindow::onMoveElementUp()
{
    QListWidgetItem* cur = ui->selected_elements->currentItem();
    if (cur)
    {
        int r=ui->selected_elements->currentRow();
        if (r>0)
        {
            QListWidgetItem* ci = ui->selected_elements->takeItem(r);
            ui->selected_elements->insertItem(r - 1, ci);
            ui->selected_elements->setCurrentRow(r-1);
        }
    }
}

void isofCaseBuilderWindow::onMoveElementDown()
{
    QListWidgetItem* cur = ui->selected_elements->currentItem();
    if (cur)
    {
        int r=ui->selected_elements->currentRow();
        if (r < ui->selected_elements->count())
        {
            QListWidgetItem* ci = ui->selected_elements->takeItem(r);
            ui->selected_elements->insertItem(r + 1, ci);
            ui->selected_elements->setCurrentRow(r+1);
        }
    }
}


void isofCaseBuilderWindow::updateTitle()
{
  QString title="InsightCAE OpenFOAM Case Builder";
  if (!current_config_file_.empty())
  {
    title+=QString(": ")+current_config_file_.c_str();
  }
  if (config_is_modified_)
  {
    title+="*";
  }
  this->setWindowTitle(title);
}

bool isofCaseBuilderWindow::CADisCollapsed() const
{
  QList<int> sz = ui->splitter_5->sizes();
  return sz[0]==0 && sz[1]==0;
}


void isofCaseBuilderWindow::onSaveAs()
{
    
//    QString fn = QFileDialog::getSaveFileName
//                 (
//                     this,
//                     "Save Parameters",
//                     "",
//                     "Insight Case Builder Parameter File (*.iscb)"
//                 );

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
      onSave();
    }
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
      OFEnode->append_attribute(doc.allocate_attribute("name", ui->OFversion->currentText().toStdString().c_str()));
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
    for (int i=0; i < ui->selected_elements->count(); i++)
    {
        InsertedCaseElement* elem = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->item(i));
        if (elem)
        {
            xml_node<> *elemnode = doc.allocate_node ( node_element, "OpenFOAMCaseElement" );
            elemnode->append_attribute(doc.allocate_attribute("type", elem->type_name().c_str()));
            rootnode->append_node ( elemnode );

            if (pack_config_file_) elem->parameters().packExternalFiles();
            elem->parameters().appendToNode(doc, *elemnode, file.parent_path());
        }
    }

    if (ui->patch_list->count())
    {
        // insert configured patches
        xml_node<> *BCnode = doc.allocate_node ( node_element, "BoundaryConditions" );
        rootnode->append_node ( BCnode );

        xml_node<> *unassignedBCnode = doc.allocate_node ( node_element, "UnassignedPatches" );
        DefaultPatch *dp = dynamic_cast<DefaultPatch*>(ui->patch_list->item(0));
        if (!dp)
        {
            throw insight::Exception("Internal error: expected default patch config node!");
        }
        if (pack_config_file_) dp->parameters().packExternalFiles();
        dp->appendToNode(doc, *unassignedBCnode, file.parent_path());
        BCnode->append_node ( unassignedBCnode );

        for (int i=1; i < ui->patch_list->count(); i++)
        {
            xml_node<> *patchnode = doc.allocate_node ( node_element, "Patch" );
            Patch *p = dynamic_cast<Patch*>(ui->patch_list->item(i));
            if (pack_config_file_) p->parameters().packExternalFiles();
            p->appendToNode(doc, *patchnode, file.parent_path());
            BCnode->append_node ( patchnode );
        }
    }

    {
        std::ofstream f ( file.c_str() );
        f << doc << std::endl;
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
    
    QString fn = QFileDialog::getOpenFileName
                 (
                     this,
                     "Load Parameters",
                     "",
                     "Insight Case Builder Parameter File (*.iscb)"
                 );

    if ( !fn.isEmpty() )
    {
        boost::filesystem::path file (fn.toStdString());
        ui->selected_elements->clear();
        loadFile(file);
    }
}

void isofCaseBuilderWindow::onParseBF()
{
    insight::OFDictData::dict boundaryDict;

    ofc_->parseBoundaryDict(casepath(), boundaryDict);
    ui->patch_list->clear();
    new DefaultPatch(ui->patch_list);
    for (const OFDictData::dict::value_type& bde: boundaryDict)
    {
//         unhandledPatches.insert(bde.first);
        new Patch(ui->patch_list, bde.first);
    }
}

void isofCaseBuilderWindow::onAddPatchManually()
{
    QString pname = QInputDialog::getText(this, "Insert Patch", "Specify patch name:");
    if (!pname.isEmpty())
    {
        new Patch(ui->patch_list, pname.toStdString());
    }
}

void isofCaseBuilderWindow::onAssignBC()
{
    QListWidgetItem *curbctype=ui->bc_element_list->currentItem();
    Patch *curpatch = dynamic_cast<Patch*>(ui->patch_list->currentItem());
    if (curbctype && curpatch)
    {
        std::string type_name = curbctype->text().toStdString();
        curpatch->set_bc_type(type_name);
        onPatchSelectionChanged();
    }
}


void isofCaseBuilderWindow::onPatchSelectionChanged()
{
    Patch* cur = dynamic_cast<Patch*>(ui->patch_list->currentItem());
    if (cur)
    {
        if (bc_ped_)
        {
          last_bc_pe_state_ = bc_ped_->saveState();
          bc_ped_->deleteLater();
        }
        bc_ped_ = new ParameterEditorWidget(cur->parameters(), ui->bc_parameter_editor);
        bc_pe_layout_->addWidget(bc_ped_);

        if (!last_bc_pe_state_.isEmpty())
        {
          bc_ped_->restoreState(last_bc_pe_state_);
        }
    //     ui->parameter_editor->setCentralWidget(ped_);
        
    //     ParameterSet emptyps;
    //     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
    }
}

void isofCaseBuilderWindow::onCleanCase()
{
  recreateOFCase(ui->OFversion->currentText());

  insight::OpenFOAMCaseDirs files(*ofc_, casepath());

  std::set<boost::filesystem::path> cands = files.caseFilesAndDirs();
  QMessageBox msg;
  msg.addButton(QMessageBox::Yes);
  msg.addButton(QMessageBox::No);
  msg.addButton(QMessageBox::Cancel);
  msg.setText("The following files and directories will be deleted. Please confirm!");
  QString list;
  for (const auto & c: cands)
    list+=QString(c.filename().c_str())+"\n";
  msg.setDetailedText(list);
  if (msg.exec() == QMessageBox::Yes)
  {
    files.cleanCase();
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

ParameterSet& isofCaseBuilderWindow::caseElementParameters(int id)
{
    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->item(id));
    if (!cur)
        throw insight::Exception
        (
            boost::str(boost::format("Error: Requested case element #%d is not valid!")%id)
        );
    
    return cur->parameters();
}

ParameterSet& isofCaseBuilderWindow::BCParameters(const std::string& patchName)
{
    QList<QListWidgetItem *>  items 
      = ui->patch_list->findItems(QString(patchName.c_str()), Qt::MatchStartsWith);
    if (items.size()<1)
        throw insight::Exception
        (
            "Error: patch \""+patchName+"\" was not found!"
        );
    if (items.size()>1)
        throw insight::Exception
        (
            "Error: patch name \""+patchName+"\" matches multiple entries!"
        );
    
    Patch* cur = dynamic_cast<Patch*>(items[0]);
    if (!cur)
        throw insight::Exception
        (
            "Error: Requested patch \""+patchName+"\" has no valid configuration!"
        ); 
    
    return cur->parameters();
}


void isofCaseBuilderWindow::selectCaseDir()
{
  QString dir = QFileDialog::getExistingDirectory(this, "Select Case Directory");
  if (!dir.isEmpty())
    ui->case_dir->setText(dir);
}


