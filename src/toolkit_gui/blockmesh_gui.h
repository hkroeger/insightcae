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

#ifndef BLOCKMESH_GUI_H
#define BLOCKMESH_GUI_H

#include "toolkit_gui_export.h"


#include "openfoam/blockmesh_templates.h"

#include "cadtypes.h"
#include "cadparametersetvisualizer.h"

namespace insight
{
namespace bmd
{



class TOOLKIT_GUI_EXPORT blockMeshDict_Box_ParameterSet_Visualizer
 : public CADParameterSetVisualizer
{
public:
    void recreateVisualizationElements() override;
};




class TOOLKIT_GUI_EXPORT blockMeshDict_Cylinder_ParameterSet_Visualizer
 : public CADParameterSetVisualizer
{
public:
    void recreateVisualizationElements() override;
    void setIcon(QIcon* i) override;
};




class TOOLKIT_GUI_EXPORT blockMeshDict_Sphere_ParameterSet_Visualizer
 : public CADParameterSetVisualizer
{
public:
    void recreateVisualizationElements() override;
};



}
}


#endif // BLOCKMESH_GUI_H
