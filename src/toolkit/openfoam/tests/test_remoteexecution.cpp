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


#include "base/boost_include.h"
#include "openfoam/remoteexecution.h"

using namespace std;
using namespace boost;
using namespace insight;

namespace fs=boost::filesystem;

int main()
{
  auto d = fs::unique_path( fs::temp_directory_path() / "remoteexec-test-%%%%%%" );
  fs::create_directory(d);
  int ret=0;

  try
  {
    {
      ofstream f( (d/"meta.foam").c_str() );
      cout << "localhost:"<<d.string()<<endl;
      f << "localhost:"<<d.string()<<endl;
    }

    RemoteExecutionConfig ec(d);

    try
    {
      ec.queueRemoteCommand("ls -l");
      ec.waitLastCommandFinished();

      ec.queueRemoteCommand("echo $SSH_CLIENT");
      ec.waitLastCommandFinished();
    }
    catch (...)
    {
      ret=-2;
    }

    ec.cancelRemoteCommands();
  }
  catch (const std::exception& e)
  {
   cerr<<e.what();
   ret=-1;
  }

  fs::remove_all(d);
  return ret;
}
