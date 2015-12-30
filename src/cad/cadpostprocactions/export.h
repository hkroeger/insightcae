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

#ifndef INSIGHT_CAD_EXPORT_H
#define INSIGHT_CAD_EXPORT_H

#include "base/boost_include.h"
#include "cadtypes.h"
#include "cadparameters.h"
#include "cadpostprocaction.h"

namespace insight 
{
namespace cad 
{

class Export 
: public PostprocAction
{
  FeaturePtr model_;
  boost::filesystem::path filename_;
  
  ScalarPtr STL_accuracy_;

  FeatureSetPtr eMesh_featureSet_;
  ScalarPtr eMesh_accuracy_;
  ScalarPtr eMesh_maxlen_;
  
public:
  Export(FeaturePtr model, const boost::filesystem::path& filename);
  Export(FeaturePtr model, const boost::filesystem::path& filename, ScalarPtr STL_accuracy);
  Export(FeatureSetPtr eMesh_featureSet, const boost::filesystem::path& filename, ScalarPtr eMesh_accuracy, ScalarPtr eMesh_maxlen);

  virtual void build();
  
  virtual AIS_InteractiveObject* createAISRepr() const;
  virtual void write(std::ostream& ) const;
};

}
}

#endif // INSIGHT_CAD_EXPORT_H
