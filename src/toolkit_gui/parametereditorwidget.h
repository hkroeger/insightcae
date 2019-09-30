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

#ifndef Q_MOC_RUN
#include "base/parameterset.h"
#endif

#include "parametersetvisualizer.h"
#include "qoccviewwidget.h"
#include "qmodeltree.h"

#undef None
#undef Bool
#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>

#include <set>
#include <memory>

class ParameterTreeWidget
    : public QTreeWidget
{
public:
  ParameterTreeWidget(QWidget* p);
  void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class ParameterSetDisplay;
namespace insight {
class CAD_ParameterSet_Visualizer;
}

class ParameterEditorWidget
: public QSplitter
{
    Q_OBJECT
    
protected:
    ParameterTreeWidget *ptree_;
    QWidget *inputContents_;

    ParameterSetDisplay* display_;

    QTreeWidgetItem* root_;
    
    insight::ParameterSet& parameters_;
    insight::ParameterSet defaultParameters_;

    insight::ParameterSet_ValidatorPtr vali_;
    std::shared_ptr<insight::CAD_ParameterSet_Visualizer> viz_;

public:

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
    ParameterEditorWidget
    (
        insight::ParameterSet& pset,
        const insight::ParameterSet& default_pset,
        QWidget* parent,
        insight::ParameterSet_VisualizerPtr viz = insight::ParameterSet_VisualizerPtr(),
        insight::ParameterSet_ValidatorPtr vali = insight::ParameterSet_ValidatorPtr(),
        ParameterSetDisplay* display = nullptr
    );
    
    void insertParameter(const QString& name, insight::Parameter& parameter, const insight::Parameter& defaultParameter);
    void doUpdateVisualization();
    
public Q_SLOTS:
    void onApply();
    void onUpdate();
    void onParameterSetChanged();

Q_SIGNALS:
    void apply();
    void update();
    void parameterSetChanged();
};



/**
 * @brief The ParameterSetDisplay class
 * Represents a union of CAD 3D display and modeltree
 * has interface to display and update multiple parameter set visualizers
 */
class ParameterSetDisplay
 : public QObject
{
  Q_OBJECT

  QoccViewWidget* viewer_;
  QModelTree* modeltree_;
  std::set<std::shared_ptr<insight::CAD_ParameterSet_Visualizer> > visualizers_;

public:
  ParameterSetDisplay
  (
      QObject* parent,
      QoccViewWidget* viewer,
      QModelTree* modeltree
  );

  inline QoccViewWidget* viewer() { return viewer_; }
  inline QModelTree* modeltree() { return modeltree_; }

  void registerVisualizer(std::shared_ptr<insight::CAD_ParameterSet_Visualizer> viz);
  void deregisterVisualizer(std::shared_ptr<insight::CAD_ParameterSet_Visualizer> viz);

public Q_SLOTS:
  void onUpdateVisualization();

};




#endif
