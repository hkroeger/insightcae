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

#ifndef ISOFCREATENUMERICSWINDOW_H
#define ISOFCREATENUMERICSWINDOW_H


#include <QListWidgetItem>

#include "openfoam/numericscaseelements.h"
#include "openfoam/openfoamcase.h"
#include "parametereditorwidget.h"



class InsertedCaseElement
: public QListWidgetItem
{
    std::string type_name_;
    insight::ParameterSet curp_;
    
public:
    InsertedCaseElement(QListWidget*, const std::string& type_name);
    
    inline const std::string& type_name() const { return type_name_; }
    inline insight::ParameterSet& parameters() { return curp_; }
    void insertElement(insight::OpenFOAMCase& ofc) const;
};



namespace Ui
{
class isofCaseBuilderWindow;
}




#include "ui_isofcasebuilderwindow.h"




class isofCaseBuilderWindow
: public QDialog
{
     Q_OBJECT
     
private:
    Ui::isofCaseBuilderWindow* ui;
    QHBoxLayout *pe_layout_;
    
protected:
    boost::shared_ptr<insight::OpenFOAMCase> ofc_;
    insight::ParameterSet parameters_;
    boost::shared_ptr<insight::FVNumerics> numerics_;
    ParameterEditorWidget *ped_;
  
public:
    isofCaseBuilderWindow();
    virtual ~isofCaseBuilderWindow();
    
public slots:
    void onItemSelectionChanged();
    virtual void done(int r);
    
    void onAddElement();
    void onRemoveElement();
    
    void onSave();
    void onLoad();
};




#endif
