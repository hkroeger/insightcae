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


#include "plottools.h"

namespace insight
{

void mplDataRead(std::ostream& f, const std::string& name, TabularResult::Table& data)
{
  f<<
  name<<"=np.array([\n"
  ;
  for (TabularResult::Table::const_iterator j=data.begin(); j!=data.end(); j++)
  {
    if (j!=data.begin()) f<<",\n";
    if (j->size()>0)
    {
      f<<"["<<*j->begin();
      for(std::vector<double>::const_iterator i=j->begin()+1; i!=j->end(); i++)
      {
	f<<","<<*i;
      }
      f<<"]\n";
    }
  }
  f<<"])\n";
}

void mplDataRead(std::ostream& f, const std::string& name, const arma::mat& x, const arma::mat& y)
{
  f<<
  name<<"=np.array([\n"
  ;
  for (int j=0; j<x.n_rows; j++)
  {
    f<<"["<<x(j)<<","<<y(j)<<"]";
    if (j!=x.n_rows-1) f<<",\n";
  }
  f<<"])\n";
}

}