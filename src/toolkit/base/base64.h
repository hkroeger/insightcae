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


#ifndef INSIGHT_BASE64_H
#define INSIGHT_BASE64_H

#include <memory>
#include <string>

#include <boost/filesystem.hpp>

#include "rapidxml/rapidxml.hpp"

namespace insight {

std::string base64_encode(const std::string& s);
std::string base64_encode(const boost::filesystem::path& f);
char* base64_encode(
    rapidxml::xml_document<> &doc,
    const std::string& file_content_ );

std::shared_ptr<std::string> base64_decode(const std::string& sourceBuffer);

void base64_decode(
        const char *sourceBuffer, size_t size,
        std::shared_ptr<std::string>& targetBuffer );

void base64_decode(
        const std::string& sourceBuffer,
        std::shared_ptr<std::string>& targetBuffer );

}

#endif // INSIGHT_BASE64_H
