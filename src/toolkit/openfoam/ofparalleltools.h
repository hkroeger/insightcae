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

#ifndef INSIGHT_OFPARALLELTOOLS_H
#define INSIGHT_OFPARALLELTOOLS_H

#include <set>
#include <vector>

#include <boost/filesystem.hpp>

#include "base/tools.h"
#include "oftimedirectories.h"

namespace insight
{

class OpenFOAMCase;

/**
 * check, if any file in orig is not existing in copy or newer in orig.
 */
bool checkIfAnyFileIsNewerOrNonexistent
(
    boost::filesystem::path orig,
    boost::filesystem::path copy,
    bool recursive=true
);


class ParallelTimeDirectories
{
  const OpenFOAMCase& cm_;
  boost::filesystem::path location_;
  TimeDirectoryList serTimes_;
  TimeDirectoryList proc0Times_;
  std::set<boost::filesystem::path> procDirs_;

public:
  ParallelTimeDirectories(
        const OpenFOAMCase& cm,
        const boost::filesystem::path& location
        );

   bool proc0TimeDirNeedsReconst(const boost::filesystem::path& ptdname) const;

    /**
   * @brief newParallelTimes
   * @return
   * parallel time directories, which are newer
   * or not present in serial and need reconstruction
   */
  std::set<boost::filesystem::path>
  newParallelTimes(bool filterOutInconsistent=false) const;

  /**
   * @brief latestTimeNeedsReconst
   * @return
   * check, if latest time step needs reconstruction
   */
  bool latestTimeNeedsReconst() const;

  bool isParallelTimeDirInconsistent(const boost::filesystem::path& timeDirName) const;

  /**
   * @brief reconstructNewTimeDirs
   * reconstructs all time directories in processors directories
   * which are not yet reconstructed
   */
  void reconstructNewTimeDirs() const;

};

bool checkIfReconstructLatestTimestepNeeded
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& location
);


class OpenFOAMCaseDirs
{

  boost::filesystem::path location_;
  std::set<boost::filesystem::path> sysDirs_, postDirs_, procDirs_;
  std::vector<boost::filesystem::path> timeDirs_;
  std::set<boost::filesystem::path> procTimeDirs_;
  std::set<boost::filesystem::path> subCaseDirs_;

public:
  enum TimeDirOpt { All, OnlyFirst, OnlyLast, OnlyFirstAndLast, ExceptFirst };

public:
  OpenFOAMCaseDirs
  (
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location
  );

  std::set<boost::filesystem::path> timeDirs( TimeDirOpt td = TimeDirOpt::All );

  static std::set<boost::filesystem::path> timeDirContent(
          const boost::filesystem::path& td );


  std::set<boost::filesystem::path> caseFilesAndDirs
  (
      TimeDirOpt td = TimeDirOpt::All,
      bool cleanProc = true,
      bool cleanTimes = true,
      bool cleanPost = true,
      bool cleanSys = true,
      bool cleanInconsistentParallelTimes = false,
      bool cleanSubCaseDirs = false
  );

  void packCase(const boost::filesystem::path& archive_file, TimeDirOpt td = TimeDirOpt::All);

  /**
   * @brief cleanCase
   * Removes all remainings of an OpenFOAM case (constant, system, processor*, postProcessing) from location.
   * Return a list with all directories and files, which have been deleted.
   */
  void cleanCase
  (
      TimeDirOpt td = TimeDirOpt::All,
      bool cleanProc=true,
      bool cleanTimes=true,
      bool cleanPost=true,
      bool cleanSys=true,
      bool cleanInconsistentParallelTimes = false,
      bool cleanSubCaseDirs = false
  );
};

#ifndef SWIG
std::ostream& operator<<(std::ostream& os, const std::set<boost::filesystem::path>& paths);
#endif

struct decompositionState
{
  bool hasProcessorDirectories;
  bool nProcDirsMatchesDecomposeParDict;
  bool decomposedLatestTimeIsConsistent;
  enum Location { Reconstructed, Decomposed, Both, Undefined };
  Location laterLatestTime;
  Location newerFiles;

  decompositionState(const boost::filesystem::path& casedir);
};

}

#endif // INSIGHT_OFPARALLELTOOLS_H
