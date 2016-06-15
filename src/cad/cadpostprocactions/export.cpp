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

#include "export.h"
#include "cadfeature.h"

namespace insight 
{
namespace cad 
{
  
Export::Export
(
  FeaturePtr model, 
  const boost::filesystem::path& filename,
  ExportNamedFeatures namedfeats
)
: model_(model),
  filename_(filename),
  namedfeats_(namedfeats)
{}

Export::Export(FeaturePtr model, const boost::filesystem::path& filename, ScalarPtr STL_accuracy)
: model_(model),
  filename_(filename),
  STL_accuracy_(STL_accuracy)
{
}

Export::Export(FeatureSetPtr eMesh_featureSet, const boost::filesystem::path& filename, ScalarPtr eMesh_accuracy, ScalarPtr eMesh_maxlen)
: filename_(filename),
  eMesh_featureSet_(eMesh_featureSet),
  eMesh_accuracy_(eMesh_accuracy),
  eMesh_maxlen_(eMesh_maxlen)
{
}

void Export::build()
{
  if (STL_accuracy_)
  {
    model_->exportSTL(filename_, *STL_accuracy_);
  }
  else
  {
    if (eMesh_accuracy_)
    {
      eMesh_featureSet_->model()->exportEMesh(filename_, *eMesh_featureSet_, eMesh_accuracy_->value(), eMesh_maxlen_->value());
    }
    else
    {
      model_->saveAs(filename_, namedfeats_);
    }
  }
}

AIS_InteractiveObject* Export::createAISRepr() const
{
  checkForBuildDuringAccess();
  return NULL;
}

void Export::write(std::ostream& ) const
{}


}
}