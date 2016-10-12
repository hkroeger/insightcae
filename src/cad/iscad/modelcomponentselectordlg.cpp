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

#include "modelcomponentselectordlg.h"
#include "ui_modelcomponentselectordlg.h"

#include "base/boost_include.h"
#include "modelfeature.h"

#include <QTreeWidgetItem>

void addModelsteps(const insight::cad::SubfeatureMap& m, QTreeWidgetItem* parent, const std::string& basename)
{
    BOOST_FOREACH(const insight::cad::SubfeatureMap::value_type& ms, m)
    {
        std::string thisname=basename+"."+ms.first;
        QStringList sl; sl << ms.first.c_str() << thisname.c_str();
        QTreeWidgetItem *curnode = new QTreeWidgetItem(parent, sl);
        
        std::cerr<<"adding "<<ms.first<<std::endl;
        addModelsteps( ms.second->providedSubshapes(), curnode, /*"("+*/thisname/*+")"*/ );
    }
}

ModelComponentSelectorDlg::ModelComponentSelectorDlg(const insight::cad::ModelPtr& m, QWidget* parent, Qt::WindowFlags f)
{
    ui = new Ui::ModelComponentSelectorDlg;
    ui->setupUi(this);
    
    BOOST_FOREACH(const insight::cad::Model::ModelstepTableContents::value_type& ms, m->modelsteps())
    {
        std::cerr<<"adding "<<ms.first<<std::endl;
        QStringList sl; sl << ms.first.c_str() <<  ms.first.c_str();
        QTreeWidgetItem *curnode = new QTreeWidgetItem(ui->component_tree, sl);
        ui->component_tree->addTopLevelItem(curnode);
        addModelsteps(ms.second->providedSubshapes(), curnode, ms.first );
    }
    
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

std::string ModelComponentSelectorDlg::selected() const
{
    QList<QTreeWidgetItem *> sel = ui->component_tree->selectedItems();
    if (sel.size()!=1) 
    {
        return std::string();
    }
    else
    {
        return sel[0]->text(1).toStdString();
    }
}
