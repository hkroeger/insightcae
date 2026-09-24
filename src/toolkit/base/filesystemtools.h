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


#ifndef INSIGHT_FILESYSTEMTOOLS_H
#define INSIGHT_FILESYSTEMTOOLS_H

#include <set>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "base/exception.h"

namespace insight {

/**
 * @brief sanitizeStringForFileName
 * remove all special characters, so that the string can be used as file name
 * @param s
 * @return
 */
boost::filesystem::path sanitizeStringForFileName(const std::string& s);


std::set<boost::filesystem::path>
wildcardSearch(const boost::filesystem::path& pathWithRegex);


template<typename StringType, typename StringIteratorType, typename AccessFunctionType>
StringType findUnusedLabel(
        StringIteratorType begin,
        StringIteratorType end,
        const StringType& desiredLabel,
        AccessFunctionType accessFunction,
        int maxAttempts=99
        )
{
    CurrentExceptionContext ex("finding an unused label in a list of labels");

    StringType lbl = desiredLabel;

    for (int attempt=1; attempt<maxAttempts; ++attempt)
    {
        insight::dbg()<<"attempt "<<attempt<<std::endl;

        bool found=false;
        for (auto it=begin; it!=end; ++it)
        {
            if ( accessFunction(it) == lbl )
            {
                found=true;
                break;
            }
        }
        if (found)
        {
            insight::dbg()<<"try lbl="<<lbl<<std::endl;
            lbl = desiredLabel + "_" + boost::lexical_cast<StringType>(attempt);
        }
        else
        {
            insight::dbg()<<"return lbl="<<lbl<<std::endl;
            return lbl;
        }
    }

    throw insight::Exception(
                str(boost::format("Could not find an unused label within %d attempts")
                    % maxAttempts ) );
}




template<typename StringType, typename StringIteratorType>
StringType findUnusedLabel(
        StringIteratorType begin,
        StringIteratorType end,
        const StringType& desiredLabel,
        int maxAttempts=99
        )
{
    return findUnusedLabel(begin, end, desiredLabel,
                           [](StringIteratorType it) -> StringType
                           {
                               return static_cast<StringType>(*it);
                           },
                           maxAttempts );
}


/**
 * @brief directoryIsWritable
 * checks, if a file can be created in the directory
 * @param directory
 * @return
 */
bool directoryIsWritable( const boost::filesystem::path& directory );

/**
 * @brief isInWritableDirectory
 * checks, if the given directory is writable, or,
 * if it does not exists, if the parent directory is writable
 * @param pathToTest
 * @return
 */
bool isInWritableDirectory( const boost::filesystem::path& pathToTest );


/**
 * @brief ensureFileExtension
 * adds an extension to the file name, if required
 * @param filePath
 * the file path to check
 * @param extension
 * the desired extension including the dot, e.g. ".ist"
 * @return
 */
boost::filesystem::path ensureFileExtension(const boost::filesystem::path& filePath, const std::string& extension);


/**
 * @brief ensureDefaultFileExtension
 * @param path
 * @param defaultExtension
 * add this extension, if the supplied path has none. May or may not start with a dot.
 * @return
 */
boost::filesystem::path
ensureDefaultFileExtension(
    const boost::filesystem::path& path,
    const std::string& defaultExtension );


std::string timeCodePrefix();


void copyDirectoryRecursively(
    const boost::filesystem::path& sourceDir,
    const boost::filesystem::path& destinationDir,
    bool failIfTargetExists = true );

}

#endif // INSIGHT_FILESYSTEMTOOLS_H
