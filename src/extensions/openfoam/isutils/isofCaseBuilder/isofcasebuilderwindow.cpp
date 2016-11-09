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

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"


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


isofCaseBuilderWindow::isofCaseBuilderWindow()
: QDialog(), ped_(NULL)
{
    // setup layout
    ui = new Ui::isofCaseBuilderWindow;
    ui->setupUi(this);
    pe_layout_ = new QHBoxLayout(ui->parameter_editor);
    
    // populate list of available case elements
    for (insight::OpenFOAMCaseElement::FactoryTable::const_iterator i = insight::OpenFOAMCaseElement::factories_->begin();
        i != insight::OpenFOAMCaseElement::factories_->end(); i++)
    {
        new QListWidgetItem(i->first.c_str(), ui->available_elements);
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
    
    ofc_.reset(new OpenFOAMCase(OFEs::get("OF23x")));
}




isofCaseBuilderWindow::~isofCaseBuilderWindow()
{
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
            boost::filesystem::path cwd = boost::filesystem::current_path();
            if 
            (
                QMessageBox::question
                (
                    this, 
                    "Confirm", 
                    str(format("Press OK to write the selected configuration into current directory %d!")
                        % cwd).c_str(),
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
                ofc_->createOnDisk(cwd);
                ofc_->modifyCaseOnDisk(cwd);
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
        
        for (xml_node<> *e = rootnode->first_node(); e; e = e->next_sibling())
        {
            std::string type_name(e->first_attribute("type")->value());
        
            InsertedCaseElement* ice = new InsertedCaseElement(ui->selected_elements, type_name);
            ice->parameters().readFromNode(doc, *e, file.parent_path());
        }
    }
}
