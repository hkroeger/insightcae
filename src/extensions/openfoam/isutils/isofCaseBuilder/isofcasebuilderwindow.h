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
#include "openfoam/openfoamtools.h"
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

    boost::filesystem::path current_config_file_;
    bool config_is_modified_=false;
    bool pack_config_file_=true;

    std::shared_ptr<insight::OpenFOAMCase> ofc_;
    insight::ParameterSet parameters_;
    ParameterEditorWidget *ped_, *bc_ped_;

    QByteArray last_pe_state_, last_bc_pe_state_;

    QString script_pre_, script_mesh_, script_case_;

    enum ExecutionStep
    {
      ExecutionStep_Clean,
      ExecutionStep_Pre,
      ExecutionStep_Mesh,
      ExecutionStep_Case
    };
  
    void fillCaseElementList();
    void updateTitle();

    bool CADisCollapsed() const;

    bool checkIfSaveNeeded();
    void saveToFile(const boost::filesystem::path& file);

public:
    isofCaseBuilderWindow();
    virtual ~isofCaseBuilderWindow();
    
    void loadFile(const boost::filesystem::path& file, bool skipBCs=false);

    void createCase
    (
        bool skipBCs=false, 
        const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles = std::shared_ptr<std::vector<boost::filesystem::path> >()
    );

    void run(ExecutionStep begin_with);

    void closeEvent(QCloseEvent *event);
    void readSettings();
    
    void expandCAD();
    void collapseCAD();

    template<class T>
    bool containsCE() const
    {
      insight::OpenFOAMCase ofc(insight::OFEs::get(ui->OFversion->currentText().toStdString()));
      for ( int i=0; i < ui->selected_elements->count(); i++ )
        {
          InsertedCaseElement* cur
            = dynamic_cast<InsertedCaseElement*> ( ui->selected_elements->item ( i ) );
          if ( cur )
            {
              std::auto_ptr<insight::OpenFOAMCaseElement> ce( cur->createElement(ofc) );
              if ( dynamic_cast<T*>(ce.get()) ) return true;
            }
        }
      return false;
    }

    QString applicationName() const;

    QString generateDefault_script_pre();
    QString generateDefault_script_mesh();
    QString generateDefault_script_case();

    boost::filesystem::path casepath() const;

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
    void onCreateNoBCs();

    void onConfigModification();

    void onCurrentTabChanged(int idx);
    void onEnterRecipeTab();
    void onChange_script_pre();
    void onChange_script_mesh();
    void onChange_script_case();
    void onReset_script_pre();
    void onReset_script_mesh();
    void onReset_script_case();

    void onOFVersionChanged(const QString & ofename);
    void recreateOFCase(const QString & ofename);
    
    insight::ParameterSet& caseElementParameters(int id);
    insight::ParameterSet& BCParameters(const std::string& patchName);

    void selectCaseDir();

    void runAll();
    void cleanAndRunAll();
    void runMeshAndSolver();
    void runSolver();
};




#endif
