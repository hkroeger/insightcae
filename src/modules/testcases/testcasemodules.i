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

%include "common.i"

%module(directors="1") TestcaseModules
%import "toolkit.i"

%{
#include "base/parameters/arrayparameter.h"
#include "base/parameters/doublerangeparameter.h"
#include "base/parameters/labeledarraykeyselectionparameter.h"
#include "base/parameters/labeledarrayparameter.h"
#include "base/parameters/matrixparameter.h"
#include "base/parameters/pathparameter.h"
#include "base/parameters/propertylibraryselectionparameter.h"
#include "base/parameters/selectablesubsetparameter.h"
#include "base/parameters/selectionparameter.h"
#include "base/parameters/simpledimensionedparameter.h"
#include "base/parameters/simpleparameter.h"
#include "base/parameters/spatialtransformationparameter.h"
#include "base/parameters/subsetparameter.h"

#include "base/resultelements/numericalresult.h"
#include "base/resultelements/scalarresult.h"
#include "base/resultelements/vectorresult.h"
#include "base/resultelements/tabularresult.h"
#include "base/resultelements/resultsection.h"
#include "base/resultelements/polarchart.h"
#include "base/resultelements/image.h"
#include "base/resultelements/contourchart.h"
#include "base/resultelements/comment.h"
#include "base/resultelements/chart.h"
#include "base/resultelements/attributeresulttable.h"

#include "code_aster/caexportfile.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/paraview.h"
#include "openfoam/caseelements/analysiscaseelements.h"
#include "channel.h"
#include "pipe.h"
#include "flatplatebl.h"

using namespace insight;
%}

%include "channel.h"
%include "pipe.h"
%include "flatplatebl.h"

