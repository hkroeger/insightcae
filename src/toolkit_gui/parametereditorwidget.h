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
#include "iqhierarchicaldatafilterproxymodel.h"

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
class IQParameterSetVisualizationScheduler;


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

    // might be detached and then be owned (and deleted) by another widget
    QPointer<QSplitter> splitterV_;
    QLabel* inputParametersLabel_ = nullptr;
    bool editPanelHeightUserAdjusted_=false;
    QTreeView* parameterTreeView_;
    // sits between model_ and parameterTreeView_ to hide filtered parameters
    IQHierarchicalDataFilterProxyModel* treeFilterModel_;
    QWidget *inputContents_;

    ParameterSetDisplay* display_;
    IQCADModel3DViewer* viewer_;

    QLabel* overlayText_ = nullptr;

    insight::ParameterSet_ValidatorPtr vali_;

    /**
     * @brief vizScheduler_
     * manages the (re-)computation of the visualization.
     * Created on first rebuild request.
     */
    QPointer<insight::IQParameterSetVisualizationScheduler> vizScheduler_;

    insight::PECADParameterSetVisualizerBuilder createVisualizer_;
    insight::CADParameterSetModelVisualizer::CreateGUIActionsFunctions::Function createGUIActions_;

    void setup(
        ParameterSetDisplay* display
        );

    void resizeEvent(QResizeEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    /**
     * @brief adjustEditPanelHeight
     * set the edit controls panel to its minimum height,
     * unless the user has adjusted it manually
     */
    void adjustEditPanelHeight();

    /**
     * @brief expandParameterTree
     * restore the default expansion state of the parameter tree
     */
    void expandParameterTree();

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

    ~ParameterEditorWidget();

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

    /**
     * @brief detachParameterPanel
     * remove the parameter tree view and the edit controls panel
     * from this widget, so that it can be placed elsewhere (e.g. as a tab of a wizard).
     * Afterwards, only the 3D view remains in this widget.
     * Ownership goes to the widget, into which the panel is inserted.
     * Only available with the constructor, which creates its own tree view.
     * @return
     * the panel or null, if not available or already detached
     */
    QWidget* detachParameterPanel();

    bool isParameterPanelDetached() const;

    void setModel(QAbstractItemModel* model);

//    void clearParameterSet();
//    void resetParameterSet(
//            insight::ParameterSet& pset,
//            const insight::ParameterSet& default_pset
//            );

    inline QAbstractItemModel* model() const { return model_; }

    /**
     * @brief setParameterFilter
     * hide all parameters (and their children) in the parameter tree,
     * whose paths match the filter
     */
    void setParameterFilter(const insight::hierarchicalData::Filter& filter);
    const insight::hierarchicalData::Filter& parameterFilter() const;

    bool hasViewer() const;
    CADViewer *viewer() const;
    inline ParameterSetDisplay* display()
    { return display_; }

    void rebuildVisualization();

    /**
     * @brief upToDateSupplementedInputData
     * @return
     * supplemented input data computed by the visualizer from the current parameters.
     * Null, if the visualization is pending/running, failed or there is no visualizer.
     */
    insight::supplementedInputDataBasePtr upToDateSupplementedInputData() const;

    /**
     * @brief whenVisualizationIdle
     * call f, once the visualization of the current parameters has completed
     * (immediately, if there is nothing pending or no visualizer)
     */
    void whenVisualizationIdle(std::function<void()> f);

    /**
     * @brief cancelWaitForVisualization
     * drop the callback registered with whenVisualizationIdle
     */
    void cancelWaitForVisualization();

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
