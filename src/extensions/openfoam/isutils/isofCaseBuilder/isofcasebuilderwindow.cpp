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
#include "openfoam/boundaryconditioncaseelements.h"
#include "openfoam/openfoamtools.h"
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
  
  HierarchyLevel::iterator i=toplevel.addHierarchyLevel("Uncategorized");

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
      QTreeWidgetItem* item = new QTreeWidgetItem ( parent->parent_, QStringList() << elemName.c_str() );
//       QFont f=item->font(0); f.setBold(true); item->setFont(0, f);
    }
    
  ui->available_elements->expandItem(topitem);
}


isofCaseBuilderWindow::isofCaseBuilderWindow()
: QMainWindow(), ped_(nullptr), bc_ped_(nullptr)
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
    
    casepath_ = boost::filesystem::current_path();
    
    onOFVersionChanged(ui->OFversion->currentText());

    {
      QList<int> s;
      s << 5000 << 5000;
      ui->splitter_2->setSizes(s);
      ui->splitter_4->setSizes(s);
    }

    setWindowIcon(QIcon(":/logo_insight_cae.png"));
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

    {
      xml_node<> *OFEnode = rootnode->first_node("OFE");
      if (OFEnode)
      {
        std::string name = OFEnode->first_attribute("name")->value();
        ui->OFversion->setCurrentIndex(ui->OFversion->findText(name.c_str()));
      }
    }
    
    for (xml_node<> *e = rootnode->first_node("OpenFOAMCaseElement"); e; e = e->next_sibling("OpenFOAMCaseElement"))
    {
        std::string type_name = e->first_attribute("type")->value();
    
        InsertedCaseElement* ice = new InsertedCaseElement(ui->selected_elements, type_name);
        ice->parameters().readFromNode(doc, *e, file.parent_path());
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

    current_config_file_=file;
    config_is_modified_=false;
    updateTitle();
}


void isofCaseBuilderWindow::createCase 
(
    bool skipBCs,
    const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles
)
{
  recreateOFCase ( ui->OFversion->currentText() );

  // insert case elements
  for ( int i=0; i < ui->selected_elements->count(); i++ )
    {
      InsertedCaseElement* cur 
        = dynamic_cast<InsertedCaseElement*> ( ui->selected_elements->item ( i ) );
      if ( cur )
        {
          cur->insertElement ( *ofc_ );
        }
    }
    
  // insert BCs
  
  if (!boost::filesystem::exists(ofc_->boundaryDictPath(casepath_)))
  {
      if (!skipBCs)
        QMessageBox::warning(this, "Warning", "No boundary dictionary present: skipping BC creation!");

      skipBCs=true;
  }
  
  insight::OFDictData::dict boundaryDict;
  if ( !skipBCs )
    {
      ofc_->parseBoundaryDict ( casepath_, boundaryDict );
      
      for ( int i=0; i < ui->patch_list->count(); i++ )
        {
          Patch* cur = dynamic_cast<Patch*> ( ui->patch_list->item ( i ) );
          if ( cur )
          {
 //           if ( boundaryDict.find(cur->patch_name()) != boundaryDict.end() )
            {
              cur->insertElement ( *ofc_, boundaryDict );
            }
          }
        }
    }
  if ( ofc_->getUnhandledPatches ( boundaryDict ).size() > 0 )
    {
      throw insight::Exception ( "Incorrect case setup: There are unhandled patches. Continuing would result in an invalid boundary definition." );
    }

  ofc_->createOnDisk ( casepath_, restrictToFiles );
  if ( !restrictToFiles ) ofc_->modifyCaseOnDisk ( casepath_ );
}

void isofCaseBuilderWindow::onConfigModification()
{
  config_is_modified_=true;
  updateTitle();
}

void isofCaseBuilderWindow::onItemSelectionChanged()
{
    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->currentItem());
    if (cur)
    {
        if (ped_) ped_->deleteLater();

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
    }
}


void isofCaseBuilderWindow::onRemoveElement()
{
    QListWidgetItem* cur = ui->selected_elements->currentItem();
    if (cur)
    {
        delete cur;
    }
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

void isofCaseBuilderWindow::onSaveAs()
{
    
    QString fn = QFileDialog::getSaveFileName
                 (
                     this,
                     "Save Parameters",
                     "",
                     "Insight Case Builder Parameter File (*.iscb)"
                 );

    if ( !fn.isEmpty() ) {

      if (! (fn.endsWith(".iscb")||fn.endsWith(".ISCB")) )
        {
          fn+=".iscb";
        }

      current_config_file_=fn.toStdString();
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
        
      boost::filesystem::path file = current_config_file_;

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

      // insert selected case elements
      for (int i=0; i < ui->selected_elements->count(); i++)
      {
          InsertedCaseElement* elem = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->item(i));
          if (elem)
          {
              xml_node<> *elemnode = doc.allocate_node ( node_element, "OpenFOAMCaseElement" );
              elemnode->append_attribute(doc.allocate_attribute("type", elem->type_name().c_str()));
              rootnode->append_node ( elemnode );

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
          dp->appendToNode(doc, *unassignedBCnode, file.parent_path());
          BCnode->append_node ( unassignedBCnode );

          for (int i=1; i < ui->patch_list->count(); i++)
          {
              xml_node<> *patchnode = doc.allocate_node ( node_element, "Patch" );
              Patch *p = dynamic_cast<Patch*>(ui->patch_list->item(i));
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

      config_is_modified_=false;
      updateTitle();
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

    ofc_->parseBoundaryDict(casepath_, boundaryDict);
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
        if (bc_ped_) bc_ped_->deleteLater();
        bc_ped_ = new ParameterEditorWidget(cur->parameters(), ui->bc_parameter_editor);
        bc_pe_layout_->addWidget(bc_ped_);
    //     ui->parameter_editor->setCentralWidget(ped_);
        
    //     ParameterSet emptyps;
    //     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
    }
}

void isofCaseBuilderWindow::onCleanCase()
{
  recreateOFCase(ui->OFversion->currentText());

  insight::OpenFOAMCaseDirs files(*ofc_, casepath_);

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

void isofCaseBuilderWindow::onCreate()
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
      return;
    }
  }

  if (ui->selected_elements->count() > 0)
  {

        if
        (
            QMessageBox::question
            (
                this,
                "Confirm",
                str(format("Press OK to write the selected configuration into current directory %d!")
                    % casepath_).c_str(),
                QMessageBox::Ok|QMessageBox::Cancel
            )
            ==
            QMessageBox::Ok
        )
        {
            bool skipBCs = ui->skipBCswitch->isChecked();
            createCase(skipBCs);
        }
    }
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
