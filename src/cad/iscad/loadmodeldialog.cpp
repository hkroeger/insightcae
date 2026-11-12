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

#include "loadmodeldialog.h"
#include "ui_loadmodeldialog.h"

#include "cadtypes.h"

#include "base/translations.h"

using namespace insight;
using namespace insight::cad;
namespace fs=boost::filesystem;




LibraryLocation *LibraryModel::getParent() const
{
  return dynamic_cast<LibraryLocation*>(parent());
}




LibraryModel::LibraryModel(QObject *parent) : QObject(parent)
{}




LibraryLocation::LibraryLocation(QObject *parent) : QObject(parent)
{}




AvailableModelsModel::AvailableModelsModel()
{
  sharedModelLocations paths;
  for (const auto p: paths)
  {
    auto loc = new LibraryLocation(this);
    loc->directory_=p;
    locations_.append(loc);
    for (fs::directory_iterator i(p);
         i!=fs::directory_iterator();
         ++i)
    {
      if ( !fs::is_directory(i->status()) )
      {
          auto mp = i->path();
          auto ext = mp.extension().string();
          boost::algorithm::to_lower(ext);
          if (ext==".iscad")
          {
            auto lm = new LibraryModel(loc);
            lm->modelName_ = mp.filename().stem().string();
            loc->models_.append(lm);
          }
      }
    }
  }
}




QModelIndex AvailableModelsModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!parent.isValid())
  {
    if (row < locations_.size())
    {
      auto l = locations_[row];
      return createIndex( row, column, l );
    }
  }
  else
  {
    auto *ip=static_cast<QObject*>(parent.internalPointer());

    if (auto *i = dynamic_cast<LibraryLocation*>(ip))
    {
      if (row < i->models_.size())
      {
        auto m = i->models_[row];
        return createIndex( row, column, m );
      }
    }
  }

  return QModelIndex();
}




QModelIndex AvailableModelsModel::parent(const QModelIndex &index) const
{
  auto *ip=static_cast<QObject*>(index.internalPointer());

  if (auto *i = dynamic_cast<LibraryModel*>(ip))
  {
    return createIndex(
          locations_.indexOf(i->getParent()),
          index.column(),
          i->getParent()
          );
  }

  return QModelIndex();
}




int AvailableModelsModel::rowCount(const QModelIndex &parent) const
{
  if (!parent.isValid())
  {
    return locations_.size();
  }
  else
  {
    auto *ip=static_cast<QObject*>(parent.internalPointer());
    if (auto* l = dynamic_cast<LibraryLocation*>(ip))
    {
      return l->models_.size();
    }
  }
  return 0;
}


int AvailableModelsModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}


QVariant AvailableModelsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole)
  {
    auto *ip=static_cast<QObject*>(index.internalPointer());

    if (auto* m = dynamic_cast<LibraryModel*>(ip))
    {
      return QString::fromStdString(m->modelName_);
    }
    else if (auto* l = dynamic_cast<LibraryLocation*>(ip))
    {
      return QString::fromStdString(l->directory_.string());
    }
  }

  return QVariant();
}


QVariant AvailableModelsModel::headerData(int section, Qt::Orientation orient, int role) const
{
  if (role==Qt::DisplayRole)
  {
    if (orient==Qt::Horizontal)
    {
      if (section==0)
        return QString(_("Model"));
    }
  }

  return QVariant();
}


LoadModelDialog::LoadModelDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::LoadModelDialog)
{
  ui->setupUi(this);

  ui->modelsTree->setModel(&avm_);
  ui->modelsTree->expandAll();
}

LoadModelDialog::~LoadModelDialog()
{
  delete ui;
}

std::string LoadModelDialog::expression() const
{
  auto ci = ui->modelsTree->currentIndex();
  auto *ip=static_cast<QObject*>(ci.internalPointer());
  if (auto* m = dynamic_cast<LibraryModel*>(ip))
  {
    return "loadmodel(" + m->modelName_ + ")";
  }
  else
    return std::string();
}

