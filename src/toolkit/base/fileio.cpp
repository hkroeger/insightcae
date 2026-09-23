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


#include "fileio.h"
#include "base/exception.h"

#include <fstream>

using namespace std;
using namespace boost::filesystem;

namespace insight
{

void readStreamIntoString(istream &in, string &fileContent)
{
  in.seekg(0, std::ios::end);
  fileContent.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&fileContent[0], fileContent.size());
}

void readFileIntoString(const path &fileName, string &fileContent)
{
  std::ifstream in(fileName.string(), std::ios::in | std::ios::binary);
  if (!in)
  {
    throw insight::Exception("Could not open file "+fileName.string()+"!");
  }
  readStreamIntoString(in, fileContent);
}


void writeStringIntoFile
(
    const std::string& fileContent,
    const boost::filesystem::path& filePath
)
{
    std::ofstream file( filePath.c_str(), std::ios::out | std::ios::binary);
    if (file.good())
    {
        file.write(fileContent.c_str(), long(fileContent.size()) );
        file.close();
    }
    else
    {
      throw insight::Exception("could not write to file "+filePath.string());
    }
}


void writeStringIntoFile
(
    std::shared_ptr<std::string> fileContent,
    const boost::filesystem::path& fileName
)
{
   writeStringIntoFile(*fileContent, fileName);
}

}
