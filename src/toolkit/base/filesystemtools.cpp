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


#include "filesystemtools.h"

#include <cstdlib>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include "boost/range/iterator_range.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
namespace fs = boost::filesystem;
using namespace boost::posix_time;

namespace insight
{


std::set<boost::filesystem::path> wildcardSearch(
    const boost::filesystem::path &rootPattern )
{
    std::set<fs::path> results;

    std::vector<boost::regex> patternParts;
    for (const auto& part : rootPattern)
    {
        patternParts.emplace_back(
            boost::regex(part.string()) );
    }

    fs::path start;
    int startIdx=0;
    if (rootPattern.is_absolute())
    {
#ifdef WIN32
        start = fs::path(rootPattern.root_path());
        if (start.empty())
        {
            start = fs::current_path().root_path();
        }
        startIdx=2;
#else
        start = fs::path("/");
#endif
    }
    else
    {
        start = fs::current_path();
    }

    std::cout<<start<<std::endl;

    std::function<void(fs::path, size_t)> recurse;
    recurse = [&](fs::path current, size_t depth)
    {
        if (depth == patternParts.size())
        {
            results.insert(current);
            return;
        }

        if (!fs::exists(current) || !fs::is_directory(current))
            return;

        if (patternParts[depth].expression()==std::string("."))
        {
            recurse(current, depth + 1);
        }
        else
        {
            const boost::regex& r = patternParts[depth];

            for (fs::directory_iterator it(current), end; it != end; ++it)
            {
                const std::string name = it->path().filename().string();

                if (!boost::regex_match(name, r))
                    continue;

                recurse(it->path(), depth + 1);
            }
        }
    };

    recurse(start, startIdx);

    return results;
}


bool directoryIsWritable( const boost::filesystem::path& directoryToTest )
{
  boost::filesystem::path directory = directoryToTest;

  if (directory.empty())
    directory=".";

  insight::CurrentExceptionContext ex("checking write permissions of directory "+directory.string());

  auto testp = boost::filesystem::unique_path( directory/"%%%%%.test" );
  try
  {

    if (std::system( ("echo xx > \""+testp.string()+"\"").c_str() )!=0) return false;

//    std::ofstream f(testp.string());
//    if (!f.is_open()) return false;
//    f.close();

    if (!boost::filesystem::exists(testp)) return false;

    boost::filesystem::remove(testp);

    return true;
  }
  catch (...)
  {
    return false;
  }
}


bool isInWritableDirectory( const boost::filesystem::path& ptt )
{
    insight::CurrentExceptionContext ex("checking, if "+ptt.string()+" is in a writable location");

    namespace bf=boost::filesystem;

    if ( bf::exists( ptt ) )
    {
        if (bf::is_directory(ptt))
        {
            return directoryIsWritable(ptt);
        }
        else
        {
            return isInWritableDirectory(ptt.parent_path());
        }
    }
    else
    {
        auto pp=ptt.parent_path();
        if (!pp.empty())
        {
            return isInWritableDirectory(pp);
        }
        else
        {
            return false;
        }
    }

    throw insight::Exception("Internal error: unhandled case");
}


std::string timeCodePrefix()
{
  ptime now = second_clock::universal_time();
  static std::locale loc(std::locale::classic(), //std::cout.getloc(),
                           new boost::posix_time::time_facet("%Y%m%d%H%M%S"));
  std::ostringstream ss;
  ss.imbue(loc);
  ss << now;

  return ss.str();
}


path ensureFileExtension(const boost::filesystem::path &filePath, const std::string &extension)
{
    if (filePath.extension()!=extension)
        return path(filePath).replace_extension(extension);
    else
        return filePath;
}


boost::filesystem::path ensureDefaultFileExtension(
    const boost::filesystem::path &pth,
    const std::string &defaultExtension )
{
    if (!pth.has_extension())
    {
      boost::filesystem::path res(pth);
      res.replace_extension(defaultExtension);
      return res;
    }
    else
      return pth;
}


path sanitizeStringForFileName(const std::string &s)
{
    std::string result(s);
    for (auto& c: result)
    {
        if (!(std::isalnum(c)||(c=='_')||(c=='-')||(c=='+')))
            c='_';
    }
    boost::replace_all(result, "__", "_");
    return result;
}


void copyDirectoryRecursively(
    const path& sourceDir,
    const path& destinationDir,
    bool failIfTargetExists )
{
    if (!exists(sourceDir) || !is_directory(sourceDir))
    {
        throw std::runtime_error("Source directory " + sourceDir.string() + " does not exist or is not a directory");
    }

    if (exists(destinationDir))
    {
        if (failIfTargetExists)
        {
            throw std::runtime_error("Destination directory " + destinationDir.string() + " already exists");
        }
    }
    else if (!create_directory(destinationDir))
    {
        throw std::runtime_error("Cannot create destination directory " + destinationDir.string());
    }

    for (const auto& dirEnt : boost::make_iterator_range(directory_iterator{sourceDir}, {}))
    {
        const auto& path = dirEnt.path();
        auto relativePathStr = path.string();
        boost::replace_first(relativePathStr, sourceDir.string(), "");

        auto from = path;
        auto to = destinationDir / relativePathStr;

        insight::dbg()<<"copy "<<from<<" to "<<to<<std::endl;

        file_status s(symlink_status(from));
        if (is_symlink(s))
            copy_symlink(from, to);
        else if (is_directory(s))
        {
            copyDirectoryRecursively(from, to, failIfTargetExists);
        }
        else if (is_regular_file(s))
        {
            copy_file(from, to,
                      failIfTargetExists ? copy_option::fail_if_exists
                                         : copy_option::overwrite_if_exists);
        }
    }
}


}
