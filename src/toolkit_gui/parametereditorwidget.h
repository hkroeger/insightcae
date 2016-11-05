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

#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>


class ParameterEditorWidget
: public QSplitter
{
    Q_OBJECT
    
protected:
    QTreeWidget *ptree_;
    QWidget *inputContents_;
    QTreeWidgetItem* root_;
    
    insight::ParameterSet& parameters_;
    
public:
    ParameterEditorWidget(insight::ParameterSet& pset, QWidget* parent);
    
    void insertParameter(const QString& name, insight::Parameter& parameter);
    
public slots:
    void onApply();
    void onUpdate();
    
signals:
    void apply();
    void update();
};




#endif
