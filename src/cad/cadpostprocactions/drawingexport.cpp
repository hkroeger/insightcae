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

#include "cadfeature.h"
#include "drawingexport.h"
#include "dxfwriter.h"

namespace insight 
{
namespace cad 
{
  
DrawingExport::DrawingExport
(
  const boost::filesystem::path& file, 
  FeaturePtr model, 
  const DrawingViewDefinitions& viewdefs
)
: file_(file),
  model_(model),
  viewdefs_(viewdefs)
{}

void DrawingExport::build()
{
  Feature::Views views;
  BOOST_FOREACH(const DrawingViewDefinition& vd, viewdefs_)
  {
    bool sec=boost::get<4>(vd);
    arma::mat up;
    VectorPtr upd=boost::get<3>(vd);
    if (upd)
      up=upd->value();
    views[boost::get<0>(vd)]=model_->createView
    (
      boost::get<1>(vd)->value(),
      boost::get<2>(vd)->value(),
      sec,
      up
    );
  }
  
  {
    DXFWriter::writeViews(file_, views);
  }
}

  
AIS_InteractiveObject* DrawingExport::createAISRepr() const
{
  checkForBuildDuringAccess();
  return NULL;
}

void DrawingExport::write(std::ostream& ) const
{}

}
}