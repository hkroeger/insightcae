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

#include "scriptpositionmap.h"

#include <algorithm>


ScriptPositionMap::ScriptPositionMap()
    : byteToChar_(1, 0)
{}


ScriptPositionMap::ScriptPositionMap(const std::string& utf8Script)
{
    byteToChar_.resize(utf8Script.size()+1);

    int nextChar=0, curChar=0;
    for (std::size_t i=0; i<utf8Script.size(); ++i)
    {
        unsigned char c=utf8Script[i];
        if ((c & 0xC0) != 0x80) // not a continuation byte
        {
            curChar=nextChar;
            // characters outside the BMP need two UTF-16 code units
            nextChar += (c>=0xF0) ? 2 : 1;
        }
        byteToChar_[i]=curChar;
    }
    byteToChar_.back()=nextChar;
}


int ScriptPositionMap::toChar(long bytePos) const
{
    bytePos=std::max(0L, std::min(bytePos, byteSize()));
    return byteToChar_[bytePos];
}


long ScriptPositionMap::toByte(int charPos) const
{
    // the first byte of a character comes first
    auto it=std::lower_bound(byteToChar_.begin(), byteToChar_.end(), charPos);
    if (it==byteToChar_.end()) return byteSize();
    return long(it-byteToChar_.begin());
}
