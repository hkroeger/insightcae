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


#ifndef INSIGHT_TEMPORARYFILE_H
#define INSIGHT_TEMPORARYFILE_H

#include <memory>
#include <fstream>

#include <boost/filesystem.hpp>

namespace insight {

class GlobalTemporaryDirectory
    : public boost::filesystem::path
{

  GlobalTemporaryDirectory();

  static std::unique_ptr<GlobalTemporaryDirectory> td_;

public:

  static const GlobalTemporaryDirectory& path();

  /**
   * @brief clear
   * Removes the temporary directory and all its contents.
   * This is only provided for use in test programs.
   * Cleanup is intendend to be done automatically at program exit.
   */
  static void clear();

  ~GlobalTemporaryDirectory();

};


class TemporaryFile
{
  boost::filesystem::path tempFilePath_;
  std::unique_ptr<std::ofstream> stream_;

  TemporaryFile(const TemporaryFile& other); // forbid copies

public:
  TemporaryFile(const std::string& fileNameModel="%%%%%.dat", const boost::filesystem::path& baseDir=boost::filesystem::path());
  ~TemporaryFile();

  std::ostream& stream();
  void closeStream();
  const boost::filesystem::path& path() const;
};

}

#endif // INSIGHT_TEMPORARYFILE_H
