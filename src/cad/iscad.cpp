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

#include <QtGui/QApplication>
#include "base/boost_include.h"

#include "base/exception.h"
#include "iscadapplication.h"

#include <QMainWindow>

#include "parser.h"

int main(int argc, char** argv)
{
  if (argc==3) 
  {
    if (std::string(argv[1])!="-batch")
    {
      cout<<"Invalid command line!"<<endl;
      exit(-1);
    }
    insight::cad::parser::Model::Ptr model(new insight::cad::parser::Model);
    return insight::cad::parseISCADModelFile(argv[2], model);
  }
  else
  {
    ISCADApplication app(argc, argv);
    ISCADMainWindow w;
    if (argc==2) 
      w.loadFile(argv[1]);
    w.show();
    return app.exec();
  }
}