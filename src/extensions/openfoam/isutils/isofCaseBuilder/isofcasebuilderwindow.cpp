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

#include "isofcasebuilderwindow.h"

#ifndef Q_MOC_RUN
#include "openfoam/boundaryconditioncaseelements.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#endif

using namespace insight;
using namespace boost;
using namespace rapidxml;




InsertedCaseElement::InsertedCaseElement(QListWidget* parent, const std::string& type_name)
: QListWidgetItem(parent), type_name_(type_name)
{
    curp_ = insight::OpenFOAMCaseElement::defaultParameters(type_name);
    setText(type_name.c_str());
}


void InsertedCaseElement::insertElement(insight::OpenFOAMCase& c) const
{
    c.insert(insight::OpenFOAMCaseElement::lookup(type_name_, c, curp_));
}




Patch::Patch(QListWidget*parent, const std::string& patch_name)
: QListWidgetItem(parent), patch_name_(patch_name)
{
    setText(patch_name_.c_str());
}


Patch::Patch(QListWidget*parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath)
: QListWidgetItem(parent)
{
    patch_name_ = node.first_attribute ( "patchName" )->value();
    setText(patch_name_.c_str());
    bc_type_ = node.first_attribute ( "BCtype" )->value();
    if (bc_type_!="")
    {
        set_bc_type(bc_type_);
        curp_.readFromNode(doc, node, inputfilepath);
    }
}


void Patch::set_bc_type(const std::string& type_name)
{
    bc_type_=type_name;
    setText( (patch_name_+" ("+bc_type_+")").c_str() );
    curp_ = BoundaryCondition::defaultParameters(bc_type_);
}

bool Patch::insertElement(insight::OpenFOAMCase& c, insight::OFDictData::dict& boundaryDict) const
{
    if (bc_type_!="")
    {
        c.insert(insight::BoundaryCondition::lookup(bc_type_, c, patch_name_, boundaryDict, curp_));
        return true;
    }
    else
    {
        return false;
    }
}


void Patch::appendToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath )
{
//     xml_node<> *elemnode = doc.allocate_node ( node_element, "OpenFOAMCaseElement" );
    node.append_attribute ( doc.allocate_attribute ( "patchName", patch_name_.c_str() ) );
    node.append_attribute ( doc.allocate_attribute ( "BCtype", bc_type_.c_str() ) );

    curp_.appendToNode ( doc, node, inputfilepath.parent_path() );
}



DefaultPatch::DefaultPatch(QListWidget* parent)
: Patch(parent, "[Unassigned Patches]")
{
}

DefaultPatch::DefaultPatch(QListWidget*parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath)
: Patch(parent, doc, node, inputfilepath)
{
}

bool DefaultPatch::insertElement ( insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict ) const
{
  if ( bc_type_!="" )
    {
      ofc.addRemainingBCs ( bc_type_, boundaryDict, curp_ );
      return true;
    }
  else
    {
      return false;
    }
}



isofCaseBuilderWindow::isofCaseBuilderWindow()
: QDialog(), ped_(NULL), bc_ped_(NULL)
{
    // setup layout
    ui = new Ui::isofCaseBuilderWindow;
    ui->setupUi(this);
    pe_layout_ = new QHBoxLayout(ui->parameter_editor);
    bc_pe_layout_ = new QHBoxLayout(ui->bc_parameter_editor);
    
    // populate list of available case elements
    for (insight::OpenFOAMCaseElement::FactoryTable::const_iterator i = insight::OpenFOAMCaseElement::factories_->begin();
        i != insight::OpenFOAMCaseElement::factories_->end(); i++)
    {
        new QListWidgetItem(i->first.c_str(), ui->available_elements);
    }
    
    // populate list of available boundary condition elements
    for (insight::BoundaryCondition::FactoryTable::const_iterator i = insight::BoundaryCondition::factories_->begin();
        i != insight::BoundaryCondition::factories_->end(); i++)
    {
        new QListWidgetItem(i->first.c_str(), ui->bc_element_list);
    }
    
    QObject::connect 
    ( 
        ui->selected_elements, SIGNAL ( itemSelectionChanged() ),
        this, SLOT ( onItemSelectionChanged() ) 
    );
    
    connect(ui->add_btn, SIGNAL(clicked()), this, SLOT(onAddElement()));
    connect(ui->remove_btn, SIGNAL(clicked()), this, SLOT(onRemoveElement()));

    connect(ui->create_btn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->cancel_btn, SIGNAL(clicked()), this, SLOT(reject()));

    connect(ui->save_btn, SIGNAL(clicked()), this, SLOT(onSave()));
    connect(ui->load_btn, SIGNAL(clicked()), this, SLOT(onLoad()));

    connect(ui->parse_bf_btn, SIGNAL(clicked()), this, SLOT(onParseBF()));
    connect(ui->assign_bc_btn, SIGNAL(clicked()), this, SLOT(onAssignBC()));
    
    QObject::connect 
    ( 
        ui->patch_list, SIGNAL ( itemSelectionChanged() ),
        this, SLOT ( onPatchSelectionChanged() ) 
    );
    
    casepath_ = boost::filesystem::current_path();
    ofc_.reset(new OpenFOAMCase(OFEs::get("OF23x")));
}




isofCaseBuilderWindow::~isofCaseBuilderWindow()
{
}


void isofCaseBuilderWindow::loadFile(const boost::filesystem::path& file)
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
    
    for (xml_node<> *e = rootnode->first_node("OpenFOAMCaseElement"); e; e = e->next_sibling("OpenFOAMCaseElement"))
    {
        std::string type_name = e->first_attribute("type")->value();
    
        InsertedCaseElement* ice = new InsertedCaseElement(ui->selected_elements, type_name);
        ice->parameters().readFromNode(doc, *e, file.parent_path());
    }

    xml_node<> *BCnode = rootnode->first_node("BoundaryConditions");
    if (BCnode)
    {
        bool bdp=true;
        try
        {
            // parse boundary information
            ofc_->parseBoundaryDict(casepath_, boundaryDict_);
        }
        catch (...)
        {
            bdp=false;
        }
            
        if (bdp)
        {
            xml_node<> *unassignedBCnode = BCnode->first_node( "UnassignedPatches" );
            new DefaultPatch(ui->patch_list, doc, *unassignedBCnode, file.parent_path());
            
            for (xml_node<> *e = BCnode->first_node("Patch"); e; e = e->next_sibling("Patch"))
            {
                new Patch(ui->patch_list, doc, *e, file.parent_path());
            }
        }
        else
        {
            QMessageBox::information(this, "boundary file", "The boundary file could not be parsed! Skipping BC configuration.");
        }
    }
}


void isofCaseBuilderWindow::onItemSelectionChanged()
{
    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->currentItem());
    if (cur)
    {
        if (ped_) ped_->deleteLater();
        ped_ = new ParameterEditorWidget(cur->parameters(), ui->parameter_editor);
        pe_layout_->addWidget(ped_);
    //     ui->parameter_editor->setCentralWidget(ped_);
        
    //     ParameterSet emptyps;
    //     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
    }
}


void isofCaseBuilderWindow::onAddElement()
{
    QListWidgetItem* cur = ui->available_elements->currentItem();
    if (cur)
    {
        std::string type_name = cur->text().toStdString();
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


void isofCaseBuilderWindow::done(int r)
{
  if ( r == QDialog::Accepted)
  {
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
                for (int i=0; i < ui->selected_elements->count(); i++)
                {
                    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->item(i));
                    if (cur)
                    {
                        cur->insertElement(*ofc_);
                    }
                }
                for (int i=0; i < ui->patch_list->count(); i++)
                {
                    Patch* cur = dynamic_cast<Patch*>(ui->patch_list->item(i));
                    if (cur)
                    {
                        cur->insertElement(*ofc_, boundaryDict_);
                    }
                }
                ofc_->createOnDisk(casepath_);
                ofc_->modifyCaseOnDisk(casepath_);
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
  }
  
  QDialog::done(r);
}




void isofCaseBuilderWindow::onSave()
{
    
    QString fn = QFileDialog::getSaveFileName
                 (
                     this,
                     "Save Parameters",
                     "",
                     "Insight Case Builder Parameter File (*.iscb)"
                 );

    if ( !fn.isEmpty() ) {
        
        boost::filesystem::path file (fn.toStdString());

        xml_document<> doc;

        // xml declaration
        xml_node<>* decl = doc.allocate_node ( node_declaration );
        decl->append_attribute ( doc.allocate_attribute ( "version", "1.0" ) );
        decl->append_attribute ( doc.allocate_attribute ( "encoding", "utf-8" ) );
        doc.append_node ( decl );

        xml_node<> *rootnode = doc.allocate_node ( node_element, "root" );
        doc.append_node ( rootnode );

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
    }
}




void isofCaseBuilderWindow::onLoad()
{
    
    ui->selected_elements->clear();
    
    QString fn = QFileDialog::getOpenFileName
                 (
                     this,
                     "Save Parameters",
                     "",
                     "Insight Case Builder Parameter File (*.iscb)"
                 );

    if ( !fn.isEmpty() ) {
                
        boost::filesystem::path file (fn.toStdString());

        loadFile(file);
    }
}

void isofCaseBuilderWindow::onParseBF()
{
    ofc_->parseBoundaryDict(casepath_, boundaryDict_);
    ui->patch_list->clear();
    new DefaultPatch(ui->patch_list);
    BOOST_FOREACH(const OFDictData::dict::value_type& bde, boundaryDict_)
    {
//         unhandledPatches.insert(bde.first);
        new Patch(ui->patch_list, bde.first);
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
