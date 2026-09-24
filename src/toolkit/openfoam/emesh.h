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

#ifndef INSIGHT_EMESH_H
#define INSIGHT_EMESH_H

#include <ostream>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>

#include "base/linearalgebra.h"

namespace insight
{

typedef std::vector<arma::mat> EMeshPtsList;
typedef std::vector<EMeshPtsList> EMeshPtsListList;


class eMesh
{
public:
    typedef std::pair<int, int> Edge;
    typedef std::vector<arma::mat> PointList;
    typedef std::vector<Edge> EdgeList;

protected:
    PointList points_;
    EdgeList edges_;

public:
    eMesh();
    eMesh(const EMeshPtsList& pts);
    eMesh(const EMeshPtsListList& pts);
    eMesh(
        const PointList&pts,
        const EdgeList&edges );

    int nPoints() const;
    int nEdges() const;
    void write(std::ostream& os) const;
    void write(const boost::filesystem::path& filename) const;

    inline const PointList& points() const { return points_; }
    inline const EdgeList& edges() const { return edges_; }
};

#ifndef SWIG
std::ostream &operator<<(std::ostream& os, const eMesh& emesh);
#endif

void exportEMesh(const EMeshPtsList& pts, const boost::filesystem::path& filename);
void exportEMesh(const EMeshPtsListList& pts, const boost::filesystem::path& filename);

}

#endif // INSIGHT_EMESH_H
