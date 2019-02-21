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

#include <QtGui>
#include <QWidget>
#include <QMainWindow>
#include <QListWidgetItem>

#ifndef Q_MOC_RUN
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamcase.h"
#endif

#include "parametereditorwidget.h"
#include "insertedcaseelement.h"
#include "patch.h"







namespace Ui
{
class isofCaseBuilderWindow;
}



#include "ui_isofcasebuilderwindow.h"




class isofCaseBuilderWindow
: public QMainWindow
{
     Q_OBJECT
     
private:
    Ui::isofCaseBuilderWindow* ui;
    QHBoxLayout *pe_layout_, *bc_pe_layout_;

    QoccViewerContext* context_;
    QoccViewWidget* viewer_;
    QModelTree* modeltree_;

protected:
    boost::filesystem::path casepath_;

    boost::filesystem::path current_config_file_;
    bool config_is_modified_=false;

    std::shared_ptr<insight::OpenFOAMCase> ofc_;
    insight::ParameterSet parameters_;
    std::shared_ptr<insight::FVNumerics> numerics_;
    ParameterEditorWidget *ped_, *bc_ped_;

    QByteArray last_pe_state_, last_bc_pe_state_;
  
    void fillCaseElementList();
    void updateTitle();

    bool CADisCollapsed() const;
    
public:
    isofCaseBuilderWindow();
    virtual ~isofCaseBuilderWindow();
    
    void loadFile(const boost::filesystem::path& file, bool skipBCs=false);
    void createCase
    (
        bool skipBCs=false, 
        const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles = std::shared_ptr<std::vector<boost::filesystem::path> >()
    );

    void closeEvent(QCloseEvent *event);
    void readSettings();
    
    void expandCAD();
    void collapseCAD();

public slots:
    void onItemSelectionChanged();
    
    void onAddElement();
    void onRemoveElement();
    void onMoveElementUp();
    void onMoveElementDown();
    
    void onSaveAs();
    void onSave();
    void onLoad();
    void onParseBF();
    void onAddPatchManually();
    void onAssignBC();
    void onPatchSelectionChanged();

    void onCleanCase();
    void onCreate();

    void onConfigModification();
    
    void onOFVersionChanged(const QString & ofename);
    void recreateOFCase(const QString & ofename);
    
    insight::ParameterSet& caseElementParameters(int id);
    insight::ParameterSet& BCParameters(const std::string& patchName);
};




#endif
