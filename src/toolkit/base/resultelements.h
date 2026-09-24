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
 * Convenience aggregator: pulls in every concrete result-element type.
 * Consumers that only need the ResultSet/ResultElement types themselves
 * should include base/resultset.h (or base/resultelementcollection.h)
 * directly instead of this file.
 *
 */


#ifndef INSIGHT_RESULTELEMENTS_H
#define INSIGHT_RESULTELEMENTS_H

#include "base/resultelements/resultsection.h"
#include "base/resultelements/comment.h"
#include "base/resultelements/numericalresult.h"
#include "base/resultelements/scalarresult.h"
#include "base/resultelements/vectorresult.h"
#include "base/resultelements/image.h"
#include "base/resultelements/video.h"
#include "base/resultelements/attributeresulttable.h"
#include "base/resultelements/tabularresult.h"
#include "base/resultelements/chart.h"
#include "base/resultelements/polarchart.h"
#include "base/resultelements/contourchart.h"

#endif // INSIGHT_RESULTELEMENTS_H
