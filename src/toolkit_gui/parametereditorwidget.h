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

#ifndef PARAMETEREDITORWIDGET_H
#define PARAMETEREDITORWIDGET_H

#include "toolkit_gui_export.h"


#ifndef Q_MOC_RUN
#include "base/parameterset.h"
#endif

#include "cadparametersetvisualizer.h"

#include "base/progressdisplayer.h"

#include "iqparametersetmodel.h"

#undef None
#undef Bool
#include <QWidget>
#include <QSplitter>
#include <QThread>
#include <QTreeView>
#include <QLabel>
#include <QPointer>

#include <set>
#include <memory>


class VisualizerThread;
class IQVTKCADModel3DViewer;
class IQVTKParameterSetDisplay;

namespace insight {
class CADParameterSetModelVisualizer;


typedef
    std::function< CADParameterSetModelVisualizer*(
        QObject*,
        IQParameterSetModel *
        )> PECADParameterSetVisualizerBuilder;

}

class TOOLKIT_GUI_EXPORT ParameterEditorWidget
: public QSplitter
{
    Q_OBJECT

public:
    typedef IQVTKParameterSetDisplay ParameterSetDisplay;
    typedef IQVTKCADModel3DViewer CADViewer;
    


protected:
//    insight::ParameterSet defaultParameters_;
    QAbstractItemModel* model_;

    QSplitter* splitterV_=nullptr;
    QTreeView* parameterTreeView_;
    QWidget *inputContents_;

    ParameterSetDisplay* display_;
    IQCADModel3DViewer* viewer_;

    QLabel* overlayText_;

    insight::ParameterSet_ValidatorPtr vali_;

    /**
     * @brief viz_
     * the visualizer object
     */
    QPointer<insight::CADParameterSetModelVisualizer> viz_;

    insight::PECADParameterSetVisualizerBuilder createVisualizer_;
    insight::CADParameterSetModelVisualizer::CreateGUIActionsFunctions::Function createGUIActions_;

    void setup(
        ParameterSetDisplay* display
        );

    bool firstShowOccurred_;
    void showEvent(QShowEvent *event) override;

    void resizeEvent(QResizeEvent*) override;

public:

    ParameterEditorWidget
    (
        QWidget* parent,
        insight::PECADParameterSetVisualizerBuilder psvb,
        insight::CADParameterSetModelVisualizer::CreateGUIActionsFunctions::Function cgaf
            = insight::CADParameterSetModelVisualizer::CreateGUIActionsFunctions::Function(),
        insight::ParameterSet_ValidatorPtr vali
            = insight::ParameterSet_ValidatorPtr(),
        ParameterSetDisplay* display = nullptr
    );

    ParameterEditorWidget
    (
        QWidget* parent,
        QTreeView* parameterTreeView,
        QWidget* contentEditorFrame,
        IQCADModel3DViewer* viewer
    );

    /**
     * @brief ParameterEditorWidget
     * create editor widget and connect with existing CAD display
     * @param pset
     * @param default_pset
     * @param parent
     * @param viewwidget
     * @param modeltree
     * @param viz
     * @param vali
     */
//    ParameterEditorWidget
//    (
//        insight::ParameterSet& pset,
//        const insight::ParameterSet& default_pset,
//        QWidget* parent,
//        insight::ParameterSetVisualizerPtr viz = insight::ParameterSetVisualizerPtr(),
//        insight::ParameterSet_ValidatorPtr vali = insight::ParameterSet_ValidatorPtr(),
//        ParameterSetDisplay* display = nullptr
//    );



    bool hasVisualizer() const;

    void setModel(QAbstractItemModel* model);

//    void clearParameterSet();
//    void resetParameterSet(
//            insight::ParameterSet& pset,
//            const insight::ParameterSet& default_pset
//            );

    inline QAbstractItemModel* model() const { return model_; }

    bool hasViewer() const;
    CADViewer *viewer() const;

    void rebuildVisualization();
    
public Q_SLOTS:
    void onParameterSetChanged();

    void onItemClicked(
            const QModelIndex &item );

Q_SIGNALS:
    void parameterSetChanged();
    void updateSupplementedInputData(std::shared_ptr<insight::supplementedInputDataBase> sid);
    void adaptEditControlsLayout(QSize ns);
};




#endif
