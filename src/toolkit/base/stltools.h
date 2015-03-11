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

#ifndef INSIGHT_STLTOOLS_H
#define INSIGHT_STLTOOLS_H

#include "base/linearalgebra.h"

#include "base/boost_include.h"

namespace insight 
{

  
class STLExtruder
{
  
  struct tri
  {
    arma::mat p[3];
    arma::mat normal() const;
  };
  
  std::vector<tri> tris_;
  
  double z0_, z1_;
  
  void addTriPair(const arma::mat& p0, const arma::mat& p1);
  void addTriTB(const arma::mat& p0, const arma::mat& p1, const arma::mat& p2);
  
  void writeTris(const boost::filesystem::path& outputfilepath);
  
public:
  STLExtruder
  (
    const arma::mat xy_contour,
    double z0, double z1,
    const boost::filesystem::path& outputfilepath
  );
  
};

}

#endif // INSIGHT_STLTOOLS_H
