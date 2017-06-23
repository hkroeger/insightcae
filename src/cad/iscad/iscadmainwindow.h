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
#include <QTimer>
#include <QTreeView>
#include <QFileSystemModel>
#include <QTabWidget>

#include "qoccviewercontext.h"
#include "qoccviewwidget.h"
#include "qdebugstream.h"
#include "viewstate.h"
#include "qmodeltree.h"

#ifndef Q_MOC_RUN
#include "pointertransient.h"
#include "cadfeaturetransient.h"
#include "occinclude.h"
#include "cadfeature.h"
#include "parser.h"
#include "sketch.h"
#include "cadmodel.h"
#endif


class ISCADModel;


class ISCADMainWindow
    : public QMainWindow
{
    Q_OBJECT

protected:
    Q_DebugStream* logger_;
    QTextEdit* log_;
    
    QTreeView* fileTree_;
    QFileSystemModel* fileModel_;
    
    QTabWidget* modelTabs_;

    QAction
        *act_load_,
        *act_save_,
        *act_saveas_,
        *act_rebuild_,
        *act_insert_feat_,
        *act_insert_component_name_,
        *act_clear_cache_,
        *act_fit_all_,
        *act_toggle_grid_,
        *act_toggle_clipxy_,
        *act_toggle_clipyz_,
        *act_toggle_clipxz_,
        *act_background_color_,
        *act_display_all_shaded_,
        *act_display_all_wire_,
        *act_reset_shading_;

    QMenu* clipplanemenu_;

    void connectMenuToModel(ISCADModel* model);
        
protected slots:
    void onFileClicked(const QModelIndex &index);
    
public slots:
    void loadModel();
    
    void activateModel(int tabindex);
    void onUpdateTabTitle(ISCADModel* model, const boost::filesystem::path& filepath, bool isUnSaved);
    void onCloseModel(int tabindex);
    void onUpdateClipPlaneMenu();
    void onLoadModelFile(const boost::filesystem::path& modelfile);
    
    void displayStatusMessage(const QString& message);

public:
    ISCADMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0, bool nolog=false);
    ~ISCADMainWindow();

    ISCADModel* insertEmptyModel();
    ISCADModel* insertModel(const boost::filesystem::path& file);
    virtual void closeEvent(QCloseEvent *event);
   
signals:
    void fileSelectionChanged(const boost::filesystem::path& file);

};


#endif // INSIGHT_CAD_ISCADMAINWINDOW_H
