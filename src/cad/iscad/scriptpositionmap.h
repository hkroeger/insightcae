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

#ifndef SCRIPTPOSITIONMAP_H
#define SCRIPTPOSITIONMAP_H

#include <string>
#include <vector>


/**
 * conversion between byte offsets in an UTF-8 encoded script
 * (as used by the parser) and character positions in the editor
 * (UTF-16 code units, as used by QString and QTextDocument)
 */
class ScriptPositionMap
{
    /**
     * editor position of the character, to which each byte belongs.
     * Contains one additional entry for the end of the script.
     */
    std::vector<int> byteToChar_;

public:
    ScriptPositionMap();
    explicit ScriptPositionMap(const std::string& utf8Script);

    /**
     * the positions are clamped to the script range
     */
    int toChar(long bytePos) const;
    long toByte(int charPos) const;

    inline long byteSize() const
    { return byteToChar_.empty() ? 0 : long(byteToChar_.size())-1; }
};


#endif // SCRIPTPOSITIONMAP_H
