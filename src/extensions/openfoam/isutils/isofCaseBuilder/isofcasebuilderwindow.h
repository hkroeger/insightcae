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
#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"
#endif

#include "iqcaditemmodel.h"
#include "iqvtkparametersetdisplay.h"
#include "parametereditorwidget.h"
#include "insertedcaseelement.h"
#include "patch.h"

#include "availablebcsmodel.h"
#include "availablecaseelementsmodel.h"
#include "boundaryconfigurationmodel.h"
#include "caseconfigurationmodel.h"





namespace Ui
{
class isofCaseBuilderWindow;
}



#include "ui_isofcasebuilderwindow.h"




class isofCaseBuilderWindow
: public QMainWindow,
      public MultivisualizationGenerator
{
     Q_OBJECT

public:
    enum ExecutionStep
    {
      ExecutionStep_Clean,
      ExecutionStep_Pre,
      ExecutionStep_Mesh,
      ExecutionStep_Case
    };

private:
    Ui::isofCaseBuilderWindow* ui;
    QHBoxLayout *pe_layout_, *bc_pe_layout_;

    //IQCADItemModel cadmodel_;
    IQVTKParameterSetDisplay* display_;

    insight::MultiCADParameterSetVisualizer::SubVisualizerList multiVizSources_;
    QPointer<insight::MultiCADParameterSetVisualizer> viz_;

    AvailableBCsModel* availableBCsModel_;
    AvailableCaseElementsModel* availableCaseElementsModel_;
    CaseConfigurationModel* caseConfigModel_;
    BoundaryConfigurationModel* BCConfigModel_;

protected:

    boost::filesystem::path current_config_file_;
    bool config_is_modified_=false;
    bool pack_config_file_=true;

    std::shared_ptr<insight::OpenFOAMCase> ofc_;

    QPointer<ParameterEditorWidget> caseElementParameterEditor_;
    QPointer<ParameterEditorWidget> patchParameterEditor_;

    // Widget geometries
    QByteArray last_pe_state_, last_bc_pe_state_;

    QString script_pre_, script_mesh_, script_case_;

    QAction *act_pack_;



    void updateTitle();

    bool CADisCollapsed() const;
    void expandOrCollapseCADIfNeeded();

    bool checkIfSaveNeeded();
    bool checkIfCaseIsEmpty();
    void saveToFile(const boost::filesystem::path& file);


public:
    isofCaseBuilderWindow();

public:
    virtual ~isofCaseBuilderWindow();
    
    void loadFile(const boost::filesystem::path& file, bool skipBCs=false);

    void createCase
    (
        bool skipBCs = false,
        const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles =
            std::shared_ptr<std::vector<boost::filesystem::path> >(),
        bool textMode = false
    );

    void run(ExecutionStep begin_with, bool skipMonitor=false);

    void closeEvent(QCloseEvent *event);
    void readSettings();


    QString generateDefault_script_pre();
    QString generateDefault_script_mesh();
    QString generateDefault_script_case();

    boost::filesystem::path casepath() const;


public Q_SLOTS:
    void onSaveAs();
    void onSave();
    void onLoad();

    void showParameterEditorForCaseElement(const QModelIndex& index);
    void showParameterEditorForPatch(const QModelIndex& index);

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

    void onTogglePacked();

    void recreateOFCase(const QString & ofename);
    
    insight::Parameter& caseElementParameter(int id, const std::string& path);
    insight::Parameter& BCParameter(const std::string& patchName, const std::string& path);

    void selectCaseDir();

    void runAll();
    void cleanAndRunAll();
    void runMeshAndSolver();
    void runSolver();

    void onStartPV();

    void setOFVersion(const QString & ofename);

    void rebuildVisualization() override;


protected Q_SLOTS:
    void onOFVersionChanged(const QString & ofename);
};




#endif
