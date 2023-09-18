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
 */

#ifndef INSIGHT_CAD_ISCADMAINWINDOW_H
#define INSIGHT_CAD_ISCADMAINWINDOW_H

#include <QMainWindow>


namespace boost { namespace filesystem { class path; } }
class IQISCADModelScriptEdit;
class IQISCADModelWindow;
class IQDebugStream;
class QTextEdit;
class QTreeView;
class QFileSystemModel;
class QMenu;
class QProgressBar;
class QPushButton;


class IQISCADMainWindow
    : public QMainWindow
{
    Q_OBJECT

protected:
    IQDebugStream* logger_;
    QTextEdit* log_;
    
    QTreeView* fileTree_;
    QFileSystemModel* fileModel_;
    
    int lastTabIndex_;
    QTabWidget* modelTabs_;

    enum ActionNames {
      save,
      saveas,
      rebuild,
      rebuild_UTC,
      insert_section_comment,
      insert_feat,
      insert_import,
      insert_component_name,
      insert_loadmodel,
      clear_cache,
      fit_all,
      toggle_grid,
      toggle_clipxy,
      toggle_clipyz,
      toggle_clipxz,
      view_plusx,
      view_minusx,
      view_plusy,
      view_minusy,
      view_plusz,
      view_minusz,
      background_color,
      display_all_shaded,
      display_all_wire,
      reset_shading,
      measure_distance,
      measure_diameter,
      sel_pts,
      sel_edgs,
      sel_faces,
      sel_solids,
      editor_font_larger,
      editor_font_smaller
    };

    std::map<ActionNames, QAction*> act_;

    QMenu* clipplanemenu_;
    QProgressBar* progressbar_;
    QPushButton* bgparsestopbtn_;

    void connectMenuToModel(IQISCADModelWindow* model, IQISCADModelWindow* lme=NULL);
        
protected slots:
    void onFileClicked(const QModelIndex &index);
    void onCreateNewModel(const QString& directory);
    void onDeleteModel(const QString& filepath);
    
public slots:
    void loadModel();
    
    void activateModel(int tabindex);
    void onUpdateTabTitle(IQISCADModelWindow* model, const boost::filesystem::path& filepath, bool isUnSaved);
    void onCloseModel(int tabindex);
    void onUpdateClipPlaneMenu(int errorState=0);
    void onNewModel();
    void onLoadModelFile(const boost::filesystem::path& modelfile);
    
    void onShowFileTreeContextMenu(const QPoint&);

    void updateProgress(int step, int totalSteps);

public:
    IQISCADMainWindow(QWidget* parent = nullptr, bool nolog=false);
    ~IQISCADMainWindow();

    IQISCADModelWindow* insertEmptyModel(bool bgparsing=true);
    IQISCADModelWindow* insertModel(const boost::filesystem::path& file, bool bgparsing=true);
    IQISCADModelWindow* insertModelScript(const std::string& contents, bool bgparsing=true);
    virtual void closeEvent(QCloseEvent *event);
    void readSettings();

signals:
    void fileSelectionChanged(const boost::filesystem::path& file);

};


#endif // INSIGHT_CAD_ISCADMAINWINDOW_H
