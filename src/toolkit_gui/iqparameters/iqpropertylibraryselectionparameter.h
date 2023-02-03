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

#ifndef IQPROPERTYLIBRARYSELECTIONPARAMETER_H
#define IQPROPERTYLIBRARYSELECTIONPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/propertylibraryselectionparameter.h"

class TOOLKIT_GUI_EXPORT IQPropertyLibrarySelectionParameter : public IQParameter
{
public:
  declareType(insight::PropertyLibrarySelectionParameter::typeName_());

  IQPropertyLibrarySelectionParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(
          IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

};
#endif // IQPROPERTYLIBRARYSELECTIONPARAMETER_H
