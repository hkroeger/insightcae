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

#include "ofparalleltools.h"
#include "openfoam/ofdictreaders.h"
#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/tools.h"
#include "base/translations.h"

#include "base/units.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/createpatch.h"

#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <map>
#include <cmath>
#include <limits>

#include "vtkSTLReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkCellData.h"
#include "base/warningdispatcher.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{


bool checkIfAnyFileIsNewerOrNonexistent
(
    boost::filesystem::path orig,
    boost::filesystem::path copy,
    bool recursive
)
{
    using namespace boost::filesystem;
    
    bool anynewerornonexistent=false;
    directory_iterator end_itr; // default construction yields past-the-end
    for 
    ( 
        directory_iterator itr( orig );
        itr != end_itr;
        ++itr 
    )
    {
        boost::filesystem::path curname=itr->path().filename();
        
        if ( is_directory(itr->status()) )
        {
            if (recursive)
            {
                anynewerornonexistent |= checkIfAnyFileIsNewerOrNonexistent(orig/curname, copy/curname);
            }
        }
        else
        {
            if (!exists(copy/curname))
            {
                std::cout<<"NOT EXISTING IN "<<copy<<": "<<curname<<std::endl;
                anynewerornonexistent = true;
            }
            else if ( last_write_time(orig/curname) > last_write_time(copy/curname) )
            {
                std::cout<<"NEWER IN "<<orig<<": "<<curname<<std::endl;
                anynewerornonexistent = true;
            }
        }
    }
    
    return anynewerornonexistent;
}

ParallelTimeDirectories::ParallelTimeDirectories(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location
    )
    : cm_(cm), location_(location)
{
    serTimes_ = listTimeDirectories(location);

    directory_iterator end_itr; // default construction yields past-the-end
    const boost::regex filter( "processor[0-9]+" );
    for ( directory_iterator i( location ); i != end_itr; i++ )
    {
        if ( is_directory(i->status()) )
        {
            std::string fn=i->path().filename().string();
            boost::smatch what;
            if ( boost::regex_match( i->path().filename().string(), what, filter ) )
            {
                procDirs_.insert(i->path());
            }
        }
    }

    auto proc0 = location/"processor0";
    if (boost::filesystem::exists(proc0)
        && boost::filesystem::is_directory(proc0))
    {
        proc0Times_=listTimeDirectories(proc0);
    }
}




bool ParallelTimeDirectories::proc0TimeDirNeedsReconst(
    const boost::filesystem::path& ptdpath ) const
{
    // search for identical time directory *name*
    auto iser=std::find_if(
        serTimes_.begin(), serTimes_.end(),
        [&](const TimeDirectoryList::value_type& td)
        {
            return boost::filesystem::basename(ptdpath)
                == boost::filesystem::basename(td.second);
        }
    );

    if (iser==serTimes_.end())
    {
        // no matching serial time dir present
        return true;
    }
    else
    {
        // its there, but up-to-date?
        return checkIfAnyFileIsNewerOrNonexistent(ptdpath, iser->second, true);
    }

    return false;
}




std::set<boost::filesystem::path> ParallelTimeDirectories::newParallelTimes(bool filterOutInconsistent) const
{
    std::set<boost::filesystem::path> result;

    for (const auto& ptd: proc0Times_)
    {
        if (proc0TimeDirNeedsReconst(ptd.second))
        {
            if ( !filterOutInconsistent
                 ||
                 (filterOutInconsistent && !isParallelTimeDirInconsistent(ptd.second.filename())) )
            {
                // no matching serial time dir present
                result.insert(ptd.second.filename());
            }
        }
    }

    return result;
}




bool ParallelTimeDirectories::latestTimeNeedsReconst() const
{
    if (proc0Times_.size())
    {
        auto lproc = (--proc0Times_.end());
        return proc0TimeDirNeedsReconst(lproc->second);
    }
    else
        return false;
}

bool ParallelTimeDirectories::isParallelTimeDirInconsistent(
    const boost::filesystem::path &ptd ) const
{
    auto j0=procDirs_.begin();
    if (!boost::filesystem::exists((*j0)/ptd))
    {
        return true;
    }
    else
    {
        auto tdc1 = OpenFOAMCaseDirs::timeDirContent((*j0)/ptd);
        for (auto j=++procDirs_.begin(); j!=procDirs_.end(); ++j)
        {
            if (!boost::filesystem::exists((*j)/ptd))
            {
                return true;
            }
            else
            {
                auto tdc2=OpenFOAMCaseDirs::timeDirContent((*j)/ptd);
                if ( tdc2 != tdc1 )
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void ParallelTimeDirectories::reconstructNewTimeDirs() const
{
    auto rtds = newParallelTimes();
    rtds.erase(boost::filesystem::path("0"));

    std::vector<std::string> tdbns;
    std::transform(rtds.begin(), rtds.end(), std::back_inserter(tdbns),
                   [&](const decltype(rtds)::value_type& path) { return path.string(); } );

    if (rtds.size())
    {
        cm_.executeCommand(
            location_,
            "reconstructPar",
            { "-time", boost::join(tdbns, ",") }
            );
    }
}

bool checkIfReconstructLatestTimestepNeeded
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& location
)
{
    return ParallelTimeDirectories(cm, location)
        .latestTimeNeedsReconst();

//  using namespace boost::filesystem;
  
//  path proc0 = location/"processor0";
//  if (!exists(proc0))
//  {
//      std::cout<<"No processor directories in case "<<location.string()<<" => no reconstruct possible"<<std::endl;
//      return false; // no reconst, if no processor directories exist
//  }
  
//  // find last timestep in proc*0
//  TimeDirectoryList tdl = listTimeDirectories( proc0 );
//  boost::filesystem::path proc0latestTimeDir = tdl.rbegin()->second;
  
//  if (tdl.size()==0)
//  {
//      std::cout<<"No time directories in procesor0 => no reconstruct possible"<<std::endl;
//      return false; // no reconst, if not time dirs in proc dirs
//  }

//  path timedirname = proc0latestTimeDir.filename();
//  path latestTimeDir = location/timedirname;
  
//  if (!exists(latestTimeDir))
//  {
//      std::cout<<"Latest time directory "<<timedirname.string()<<" in procesor0 not existing in case "<<location.string()<<" => reconstruct required"<<std::endl;
//      return true; // reconst needed, if latest time is only existing in proc dir
//  }
//  else
//  {
//      // time dir exists also in case; check if files in proc dir are newer
      
//      if (checkIfAnyFileIsNewerOrNonexistent(proc0latestTimeDir, latestTimeDir, false))
//      {
//          std::cout<<"There are newer or non-existing field files in case time dir => reconstruct required"<<std::endl;
//          return true;
//      }
      
//      if (exists(proc0latestTimeDir/"polyMesh"))
//      {
//          if (!exists(latestTimeDir/"polyMesh"))
//          {
//            std::cout<<"polyMesh folder in case time dir not existing => reconstruct required"<<std::endl;
//            return true;
//          }
//          else if (checkIfAnyFileIsNewerOrNonexistent(proc0latestTimeDir/"polyMesh", latestTimeDir/"polyMesh", false))
//          {
//            std::cout<<"There are newer or non-existing files in polyMesh folder => reconstruct required"<<std::endl;
//            return true;
//          }
//      }
//  }
//  return false;
}

OpenFOAMCaseDirs::OpenFOAMCaseDirs
(
    const OpenFOAMCase&,
    const path& location
)
  : location_(location)
{
  if (!exists(location))
    throw insight::Exception("OpenFOAM case location does not exist: "+location.string());

  std::vector<path> to_test;

  to_test = { "system", "constant" };
  for (const auto& tt: to_test)
  {
    if (exists(location/tt)) sysDirs_.insert(location/tt);
  }
  to_test = { "wnow", "wnowandstop" };
  for (const auto& tt: to_test)
  {
    if (exists(location/tt)) sysDirs_.insert(location/tt);
  }

  to_test = { "postProcessing", "VTK" };
  for (const auto& tt: to_test)
  {
    if (exists(location/tt)) postDirs_.insert(location/tt);
  }

  TimeDirectoryList tdl = listTimeDirectories(location);
  for (const auto& td: tdl)
  {
    timeDirs_.push_back(td.second);
  }



  directory_iterator end_itr; // default construction yields past-the-end
  {
      const boost::regex filter( "processor[0-9]+" );
      for ( directory_iterator i( location ); i != end_itr; i++ )
      {
          if ( is_directory(i->status()) )
          {
              std::string fn=i->path().filename().string();
              boost::smatch what;
              if ( boost::regex_match( i->path().filename().string(), what, filter ) )
              {
                  procDirs_.insert(i->path());
              }
          }
      }
  }

  // get a list of every time directory processor dirs
  // also include potentially inconsistent dirs
  for (const auto& proc: procDirs_)
  {
      auto ptds = listTimeDirectories(proc);
      for (const auto& ptd: ptds)
      {
          procTimeDirs_.insert(ptd.second.filename());
      }
  }

  {
      const boost::regex filter( "^subcase__.*" );
      for ( directory_iterator i( location ); i != end_itr; i++ )
      {
          if ( is_directory(i->status()) )
          {
              std::string fn=i->path().filename().string();
              boost::smatch what;
              if ( boost::regex_match( i->path().filename().string(), what, filter ) )
              {
                  subCaseDirs_.insert(i->path());
              }
          }
      }
  }

}

std::set<boost::filesystem::path> OpenFOAMCaseDirs::timeDirs( OpenFOAMCaseDirs::TimeDirOpt td )
{
  std::set<boost::filesystem::path> tds;

  if (td==TimeDirOpt::All)
  {
    std::copy( timeDirs_.begin(), timeDirs_.end(), std::inserter(tds, tds.begin()) );
  }

  if ( td==TimeDirOpt::OnlyFirst || td==TimeDirOpt::OnlyFirstAndLast )
  {
    if (timeDirs_.size()>0) tds.insert(timeDirs_.front());
  }

  if (td==TimeDirOpt::OnlyLast || td==TimeDirOpt::OnlyFirstAndLast )
  {
    if (timeDirs_.size()>0)
    {
      if (timeDirs_.back().filename().string()!="0")
        tds.insert(timeDirs_.back());
    }
  }

  if (td==TimeDirOpt::ExceptFirst)
  {
    if (timeDirs_.size()>1)
    {
      std::copy( timeDirs_.begin()+1, timeDirs_.end(), std::inserter(tds, tds.begin()) );
    }
  }


  return tds;
}

std::set<boost::filesystem::path> OpenFOAMCaseDirs::timeDirContent(
        const boost::filesystem::path& td )
{
    std::set<boost::filesystem::path> tdc;

    for (const auto& f : recursive_directory_iterator(td))
    {
        tdc.insert(make_relative(td,f));
    }
    return tdc;
}

std::ostream& operator<<(std::ostream& os, const std::set<boost::filesystem::path>& paths)
{
    for (const auto& p: paths)
    {
        os << " "<<p.string();
    }
    return os;
}

std::set<boost::filesystem::path> OpenFOAMCaseDirs::caseFilesAndDirs
(
    OpenFOAMCaseDirs::TimeDirOpt td,
    bool cleanProc,
    bool cleanTimes,
    bool cleanPost,
    bool cleanSys,
    bool cleanInconsistentParallelTimes,
    bool cleanSubCaseDirs
)
{
  std::set<boost::filesystem::path> all_cands;

  if (cleanSys) std::copy( sysDirs_.begin(), sysDirs_.end(), std::inserter(all_cands, all_cands.begin()) );

  if (cleanPost) std::copy( postDirs_.begin(), postDirs_.end(), std::inserter(all_cands, all_cands.begin()) );

  if (cleanTimes)
  {
    auto tds = timeDirs(td);
    std::copy( tds.begin(), tds.end(), std::inserter(all_cands, all_cands.begin()) );
  }

  if (cleanProc) std::copy( procDirs_.begin(), procDirs_.end(), std::inserter(all_cands, all_cands.begin()) );

  if (cleanInconsistentParallelTimes)
  {
      std::set<boost::filesystem::path> inconsistentParTimes;

      auto j0=procDirs_.begin();
      for (const auto& ptd: procTimeDirs_)
      {
          if (!boost::filesystem::exists((*j0)/ptd))
          {
            inconsistentParTimes.insert(ptd);
          }
          else
          {
              auto tdc1 = timeDirContent((*j0)/ptd);
              for (auto j=++procDirs_.begin(); j!=procDirs_.end(); ++j)
              {
                  if (!boost::filesystem::exists((*j)/ptd))
                  {
                    inconsistentParTimes.insert(ptd);
                    break;
                  }
                  else
                  {
                      auto tdc2=timeDirContent((*j)/ptd);
                      if ( tdc2 != tdc1 )
                      {
                        inconsistentParTimes.insert(ptd);
                        break;
                      }
                  }
              }
          }
      }

      for (const auto& ptd: inconsistentParTimes)
      {
          for (const auto& pd: procDirs_)
          {
              auto dn = pd/ptd;
              if (boost::filesystem::exists(dn))
              {
                  all_cands.insert(dn);
              }
          }
      }
  }

  if (cleanSubCaseDirs)
  {
      std::copy(
          subCaseDirs_.begin(), subCaseDirs_.end(),
          std::inserter(all_cands, all_cands.begin())
          );
  }

  return all_cands;
}

void OpenFOAMCaseDirs::packCase(const boost::filesystem::path& archive_file,OpenFOAMCaseDirs::TimeDirOpt td)
{
  std::string cmd;
  cmd+="cd "+location_.string()+";";
  cmd+="tar czf "+archive_file.string();

  std::vector<std::string> filesAndDirsToPack;
  for (const auto& c: sysDirs_) filesAndDirsToPack.push_back(make_relative(location_, c).string());
  for (const auto& c: postDirs_) filesAndDirsToPack.push_back(make_relative(location_, c).string());

  auto tds = timeDirs(td);
  for (const auto& c: tds) filesAndDirsToPack.push_back(make_relative(location_, c).string());

  if (filesAndDirsToPack.size()>0)
  {

    cmd+=" "+boost::join(filesAndDirsToPack, " " );

    if (::system(cmd.c_str()) != 0)
      throw insight::Exception("Could not pack OpenFOAM case files.\n"
                               "Command was \""+cmd+"\"");
  }
  else
  {
    insight::Warning("There are no files or directories to pack. Nothing archived.");
  }
}

void OpenFOAMCaseDirs::cleanCase
(
    OpenFOAMCaseDirs::TimeDirOpt td,
    bool cleanProc,
    bool cleanTimes,
    bool cleanPost,
    bool cleanSys,
    bool cleanInconsistentParallelTimes,
    bool cleanSubCaseDirs
)
{

  auto cands = caseFilesAndDirs(
        td,
        cleanProc, cleanTimes, cleanPost, cleanSys,
        cleanInconsistentParallelTimes,
        cleanSubCaseDirs );

  for (const auto& c: cands)
  {
    std::cout<<"DELETING: "<<c<<std::endl;
    remove_all(c);
  }
}

decompositionState::decompositionState(const boost::filesystem::path& casedir)
{
  CurrentExceptionContext ce("Checking decomposition state of case in "+casedir.string());

  if (!boost::filesystem::exists(casedir))
    throw insight::Exception("Case directory "+casedir.string()+" does not exist!");

  int np=1;
  try
  {
    np=readDecomposeParDict(casedir);
  }
  catch (...) {} // cannot read decomposeParDict (not existent) => serial case

  std::vector<boost::filesystem::path> procDirs;
  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( casedir );
          itr != end_itr; itr++ )
  {
      if ( is_directory(itr->status()) )
      {
          if (starts_with(itr->path().filename().string(), "processor"))
          {
            procDirs.push_back(itr->path());
          }
      }
  }

  if (procDirs.size()>0)
    hasProcessorDirectories=true;
  else
    hasProcessorDirectories=false;

  if (procDirs.size()==size_t(np))
    nProcDirsMatchesDecomposeParDict=true;
  else
    nProcDirsMatchesDecomposeParDict=false;

  //
  // check, if the same latest time step is existing in all processor directories and
  // has everywhere the same fields
  //
  decomposedLatestTimeIsConsistent=true;
  std::string latestTime;
  std::set<std::string> fields0;
  for (auto pd=procDirs.begin(); pd!=procDirs.end(); pd++)
  {
    auto tdl = listTimeDirectories(*pd);

    if (pd==procDirs.begin())
    {
        if (tdl.size()>0)
        {
          latestTime=tdl.rbegin()->second.filename().string();
          for (const auto& f:
               boost::filesystem::directory_iterator(tdl.rbegin()->second))
          {
            // get list of fields
            fields0.insert( f.path().filename().string() );
          }
        }
    }
    else
    {
      if (tdl.size()>0)
      {
        std::string cur_latestTime=tdl.rbegin()->second.filename().string();

        if (cur_latestTime!=latestTime)
        {
          decomposedLatestTimeIsConsistent = false;
        }
        else
        {
          size_t nfound=0;
          // check, if the same fields are present
          for (const auto& f: boost::filesystem::directory_iterator(tdl.rbegin()->second))
          {
            if (fields0.find(f.path().filename().string()) != fields0.end())
            {
              nfound++;
            }
          }
          decomposedLatestTimeIsConsistent = decomposedLatestTimeIsConsistent
                                             && (nfound==fields0.size());
        }
      }
    }
  }

  if (!hasProcessorDirectories)
  {
    laterLatestTime=Location::Reconstructed;
    newerFiles=Location::Reconstructed;
  }
  else
  {
    if (decomposedLatestTimeIsConsistent)
    {
      // check, where the latest time step lies
      auto tdl = listTimeDirectories(casedir);
      if (tdl.size()==0)
      {
        if (latestTime==std::string())
        {
          // neither in decomposed nor reconst case are time steps
          laterLatestTime=Location::Both;
          newerFiles=Location::Undefined;
        }
        else
        {
          // no reconst time steps but time steps in proc dirs
          laterLatestTime=Location::Decomposed;
          newerFiles=Location::Decomposed;
        }
      }
      else
      {
        if (latestTime==std::string())
        {
          // in reconst case are time steps but not in proc dirs
          laterLatestTime=Location::Reconstructed;
          newerFiles=Location::Reconstructed;
        }
        else
        {
          double lt_decomp=toNumber<double>(latestTime);
          if ( fabs(tdl.rbegin()->first - lt_decomp) < 1e-15 )
          {
            // same time step in recon and decomp
            laterLatestTime=Location::Both;

            boost::filesystem::path
                p_dec = casedir/"processor0"/latestTime,
                p_rec = tdl.rbegin()->second;
            bool rec_newer=false, dec_newer=false;
            for (const auto& f: fields0)
            {
              if (
                  boost::filesystem::last_write_time(p_dec/f)
                  <
                  boost::filesystem::last_write_time(p_rec/f)
                 )
              {
                rec_newer=true;
              }
              else
              {
                dec_newer=true;
              }
            }
            if (rec_newer && dec_newer)
            {
              newerFiles=Location::Both;
            }
            else if (rec_newer)
            {
              newerFiles=Location::Reconstructed;
            }
            else if (dec_newer)
            {
              newerFiles=Location::Decomposed;
            }
            else
            {
              newerFiles=Location::Undefined;
            }
          }
          else if ( tdl.rbegin()->first > lt_decomp )
          {
            laterLatestTime=Location::Reconstructed;
            newerFiles=Location::Reconstructed;
          }
          else if ( tdl.rbegin()->first < lt_decomp )
          {
            laterLatestTime=Location::Decomposed;
            newerFiles=Location::Decomposed;
          }
        }
      }
    }
  }

}


}
