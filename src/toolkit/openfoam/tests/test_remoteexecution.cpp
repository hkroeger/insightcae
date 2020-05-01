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
#include "base/remoteexecution.h"

using namespace std;
using namespace boost;
using namespace insight;

namespace fs=boost::filesystem;

void test_existing_directory(bool old_format)
{
  auto d = fs::unique_path( fs::temp_directory_path() / "remoteexec-test-%%%%%%" );
  fs::create_directory(d);
  int ret=0;

  try
  {
    {
      ofstream f( (d/"meta.foam").c_str() );

      if (old_format)
      {
        f << "localhost:"<<d.string()<<endl;
      }
      else
      {
        f << "<remote server=\"localhost\" directory=\""<<d.string()<<"\"/>"<<endl;
      }
    }

    RemoteExecutionConfig ec(d);

    try
    {
      ec.queueRemoteCommand("pwd");
      ec.queueRemoteCommand("ls -la");
      ec.queueRemoteCommand("echo $SSH_CLIENT");
      ec.waitLastCommandFinished();
    }
    catch (...)
    {
      ret=2;
    }

    ec.cancelRemoteCommands();
  }
  catch (const std::exception& e)
  {
   cerr<<e.what();
   ret=1;
  }

  fs::remove_all(d);

  if (ret) throw ret;
}




int test_auto_create()
{
  auto d = fs::unique_path( fs::temp_directory_path() / "remoteexec-test-%%%%%%" );
  fs::create_directory(d);
  int ret=0;

  try
  {

    auto i=insight::remoteServers.find("localhost");

    RemoteExecutionConfig ec(i->second, d);

    try
    {
      ec.queueRemoteCommand("pwd");
      ec.queueRemoteCommand("ls -la");
      ec.queueRemoteCommand("echo $SSH_CLIENT");
      ec.waitLastCommandFinished();
    }
    catch (...)
    {
      ret=2;
    }

    ec.cancelRemoteCommands();
  }
  catch (const std::exception& e)
  {
   cerr<<e.what();
   ret=1;
  }

  fs::remove_all(d);

  if (ret) throw ret;
}




void test_start_disconnect(fs::path d)
{

  int ret=0;

  try
  {

    auto i=insight::remoteServers.find("localhost");

    RemoteExecutionConfig ec(i->second, d);

    try
    {
      ec.queueRemoteCommand("pwd");
      ec.queueRemoteCommand("ls -la");
      ec.waitLastCommandFinished();
    }
    catch (...)
    {
      ret=2;
    }
  }
  catch (const std::exception& e)
  {
   cerr<<e.what();
   ret=1;
  }

  if (ret) throw ret;

}



void test_reconnect_stop(fs::path d)
{

  int ret=0;

  try
  {

    RemoteExecutionConfig ec(d);

    try
    {
      ec.queueRemoteCommand("echo $SSH_CLIENT");
      ec.waitLastCommandFinished();
    }
    catch (...)
    {
      ret=2;
    }

    ec.cancelRemoteCommands();
  }
  catch (const std::exception& e)
  {
   cerr<<e.what();
   ret=1;
  }

  if (ret) throw ret;
}



int main()
{
  int ret=0;

  try
  {
    test_existing_directory(true);

    test_existing_directory(false);

    test_auto_create();

    {
      auto d = fs::unique_path( fs::temp_directory_path() / "remoteexec-test-%%%%%%" );
      fs::create_directory(d);

      int rtest=0;
      try
      {
        test_start_disconnect(d);
        test_reconnect_stop(d);
      }
      catch (int r)
      {
        rtest=r;
      }

      fs::remove_all(d);
      if (rtest) throw rtest;
    }
  }
  catch (int r)
  {
    ret=r;
  }

  return ret;
}
