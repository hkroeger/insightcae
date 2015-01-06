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

#include "codeasterrun.h"

using namespace std;

namespace insight
{

CAEnvironment::CAEnvironment(const boost::filesystem::path& asrun_cmd)
: asrun_cmd_(asrun_cmd)
{
}

int CAEnvironment::version() const
{
  return 11;
}

const boost::filesystem::path& CAEnvironment::asrun_cmd() const
{
  return asrun_cmd_;
}

void CAEnvironment::forkCase
(
  const boost::filesystem::path& exportfile
) const
{
  boost::filesystem::path dir=exportfile.parent_path();
  std::string cmd = std::string("cd ")+dir.c_str()+"; "+asrun_cmd().c_str()+" --run "+exportfile.filename().c_str();

  std::string sys("bash -lc '( "); sys += cmd+" ) &'";
  cout<<"Executing "<<sys<<endl;
  system(sys.c_str());
}

}