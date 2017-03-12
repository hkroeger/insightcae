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


void addScalarSymbols(const insight::cad::Feature& feat, QTreeWidgetItem* parent, const std::string& basename)
{
    BOOST_FOREACH(const insight::cad::Feature::RefValuesList::value_type& v, feat.getDatumScalars())
    {
        std::string thisname=basename+"$"+v.first;
        QStringList sl; sl << v.first.c_str() << thisname.c_str();
        QTreeWidgetItem *curnode = new QTreeWidgetItem(parent, sl);
        QFont f=curnode->font(0);
        f.setItalic(true);
        f.setPointSize(f.pointSize()-1);
        
        curnode->setFont(0, f);
        curnode->setFont(1, f);
        QBrush fb(Qt::darkCyan);
        curnode->setForeground(0, fb);
        curnode->setForeground(1, fb);
    }    
}


void addPointSymbols(const insight::cad::Feature& feat, QTreeWidgetItem* parent, const std::string& basename)
{
    QBrush fb(Qt::darkGray);
    BOOST_FOREACH(const insight::cad::Feature::RefPointsList::value_type& p, feat.getDatumPoints())
    {
        std::string thisname=basename+"@"+p.first;
        QStringList sl; sl << p.first.c_str() << thisname.c_str();
        QTreeWidgetItem *curnode = new QTreeWidgetItem(parent, sl);
        QFont f=curnode->font(0);
        f.setItalic(true);
        f.setPointSize(f.pointSize()-1);
        curnode->setFont(0, f);
        curnode->setFont(1, f);
        curnode->setForeground(0, fb);
        curnode->setForeground(1, fb);
    }
    BOOST_FOREACH(const insight::cad::Feature::RefVectorsList::value_type& v, feat.getDatumVectors())
    {
        std::string thisname=basename+"^"+v.first;
        QStringList sl; sl << v.first.c_str() << thisname.c_str();
        QTreeWidgetItem *curnode = new QTreeWidgetItem(parent, sl);
        QFont f=curnode->font(0);
        f.setItalic(true);
        f.setPointSize(f.pointSize()-1);
        
        curnode->setFont(0, f);
        curnode->setFont(1, f);
        curnode->setForeground(0, fb);
        curnode->setForeground(1, fb);
    }
}


void addSymbols(const insight::cad::Feature& feat, QTreeWidgetItem* parent, const std::string& basename)
{
    addScalarSymbols(feat, parent, basename);
    addPointSymbols(feat, parent, basename);
}


void addModelsteps
(
    const insight::cad::Feature& feat, 
    QTreeWidgetItem* parent, 
    const std::string& basename,
    std::set<std::string> components = std::set<std::string>()
)
{
    BOOST_FOREACH(const insight::cad::SubfeatureMap::value_type& ms, feat.providedSubshapes())
    {
        insight::cad::FeaturePtr fp = ms.second;
        std::string thisname=basename+"."+ms.first;
        QStringList sl; sl << ms.first.c_str() << thisname.c_str();
        QTreeWidgetItem *curnode = new QTreeWidgetItem(parent, sl);
        if (components.find(ms.first)!=components.end())
        {
            QFont f=curnode->font(0);
            f.setBold(true);
            curnode->setFont(0, f);
            curnode->setFont(1, f);
        }
        
        std::cerr<<"adding "<<ms.first<<std::endl;
        std::set<std::string> components;
        if ( insight::cad::ModelFeature* mf = dynamic_cast<insight::cad::ModelFeature*>(fp.get()) )
        {
            components = mf->model()->components();
        }
        addModelsteps( *fp, curnode, thisname, components );
        addSymbols( *ms.second, curnode, thisname );
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
        if (m->components().find(ms.first)!=m->components().end())
        {
            QFont f=curnode->font(0);
            f.setBold(true);
            curnode->setFont(0, f);
            curnode->setFont(1, f);
        }
        ui->component_tree->addTopLevelItem(curnode);
        std::set<std::string> components;
        if ( insight::cad::ModelFeature* mf = dynamic_cast<insight::cad::ModelFeature*>(ms.second.get()) )
        {
            components = mf->model()->components();
        }
        addModelsteps( *ms.second, curnode, ms.first, components );
        addSymbols( *ms.second, curnode, ms.first );
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
