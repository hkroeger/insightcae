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

#include "base/boost_include.h"

#include "cadfeature.h"

#include "insertfeaturedlg.h"
#include "ui_insertfeaturedlg.h"


using namespace insight::cad;

class Entry
: public QListWidgetItem
{
    FeatureCmdInfo info_;

public:
    Entry(const FeatureCmdInfo& info, QListWidget* parent)
    : QListWidgetItem( QString(info.command_.c_str()), parent),
      info_(info)
    {}
    
    QString command() const { return QString(info_.command_.c_str()); }
    QString signature() const { return QString(info_.signature_.c_str()).replace("<", "&lt;").replace(">", "&gt;").replace("\n", "<br>"); }
    QString signaturePlain() const { return QString(info_.signature_.c_str()); }
    QString documentation() const { return QString(info_.documentation_.c_str()); }
};

InsertFeatureDlg::InsertFeatureDlg(QWidget* parent)
{
    ui = new Ui::InsertFeatureDlg;
    ui->setupUi(this);
    
    for 
    (
        insight::cad::Feature::FactoryTable::const_iterator i = insight::cad::Feature::factories_->begin();
        i != insight::cad::Feature::factories_->end(); 
        i++
    )
    {
        insight::cad::FeaturePtr sm(i->second->operator()(insight::NoParameters()));
        insight::cad::FeatureCmdInfoList infos = sm->ruleDocumentation();
        BOOST_FOREACH(const insight::cad::FeatureCmdInfo& info, infos)
        {
            new Entry(info, ui->featureCmdList);
        }
    }
    
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
    connect(ui->featureCmdList, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
    
    connect(ui->isIntermediateStep, SIGNAL(clicked()), this, SLOT(onIsIntermediateStepActivated()));
    connect(ui->isFinalComponent, SIGNAL(clicked()), this, SLOT(onIsFinalComponentActivated()));
    connect(ui->onlyFeatureCommand, SIGNAL(clicked()), this, SLOT(onOnlyFeatureCommandActivated()));
}

void InsertFeatureDlg::onItemSelectionChanged ()
{
    QListWidgetItem * item = ui->featureCmdList->currentItem();
    if (Entry* e=dynamic_cast<Entry*>(item))
    {
        QString help;
        help=
            "<p><b>"+e->command()+e->signature()+"</b><br></p>"
            +
            "<p>"+e->documentation()+"</p>"
            ;
        ui->featureCmdHelp->setHtml(help);
    }
}

void InsertFeatureDlg::accept()
{
    QListWidgetItem *sel = ui->featureCmdList->currentItem();
    if (Entry* e=dynamic_cast<Entry*>(sel))
    {
        insert_string_ = e->command()+e->signaturePlain();
        
        QString name = ui->featureName->text();
        
        if (ui->isIntermediateStep->isChecked())
        {
            insert_string_ = name + "=\n" + insert_string_ + "\n;\n";
        }
        else if (ui->isFinalComponent->isChecked())
        {
            insert_string_ = name + ":\n" + insert_string_ + "\n;\n";
        }
    }
    QDialog::accept();
}

void InsertFeatureDlg::onIsIntermediateStepActivated()
{
    ui->featureNameLabel->setEnabled(true);
    ui->featureName->setEnabled(true);
}

void InsertFeatureDlg::onIsFinalComponentActivated()
{
    ui->featureNameLabel->setEnabled(true);
    ui->featureName->setEnabled(true);
}

void InsertFeatureDlg::onOnlyFeatureCommandActivated()
{
    ui->featureNameLabel->setEnabled(false);
    ui->featureName->setEnabled(false);
}
