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

#ifndef INSIGHT_CAD_DRAWINGEXPORT_H
#define INSIGHT_CAD_DRAWINGEXPORT_H

#include "cadparameters.h"
#include "cadpostprocaction.h"
#include "base/boost_include.h"

namespace insight 
{
namespace cad 
{

    
/**
 * whether to create the additional views:
 * 1 left
 * 2 right
 * 3 top
 * 4 bottom
 * 5 back
 */
typedef boost::fusion::vector5<bool, bool, bool, bool, bool> AdditionalViews;
    
typedef boost::tuple<std::string, VectorPtr, VectorPtr, VectorPtr, bool, bool, bool, AdditionalViews > DrawingViewDefinition;
typedef boost::fusion::vector2<FeaturePtr, std::vector<DrawingViewDefinition> > DrawingViewDefinitions;




class DrawingExport 
: public PostprocAction
{
  boost::filesystem::path file_; 
  std::vector<DrawingViewDefinitions> viewdefs_;
  TopoDS_Shape shape_;

public:
  DrawingExport
  (
    const boost::filesystem::path& file,
    std::vector<DrawingViewDefinitions> viewdefs
  );
  
  virtual void build();

  virtual Handle_AIS_InteractiveObject createAISRepr(const Handle_AIS_InteractiveContext& context) const;
  virtual void write(std::ostream& ) const;
};
}
}

#endif // INSIGHT_CAD_DRAWINGEXPORT_H
