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

#ifndef INSIGHT_CAD_PARSER_TOKENS_H
#define INSIGHT_CAD_PARSER_TOKENS_H

/*
 * information about the recognized script elements,
 * which is recorded by the ISCAD parser
 * (lightweight, without dependency on the parser itself)
 */

#include <string>
#include <utility>


namespace insight {
namespace cad {
namespace parser {


typedef std::pair<long, long> SyntaxElementPos;


enum class CommandKind { FeatureCommand, ScalarFunction, VectorFunction, PostprocCommand };


/**
 * classification of a recognized piece of script text
 * (e.g. for syntax highlighting)
 */
enum class TokenKind
{
    FeatureCommand, ScalarFunction, VectorFunction, PostprocCommand,

    // symbol definitions (left hand side of assignments)
    FeatureDef, ComponentDef, ScalarDef, VectorDef, DatumDef, SelectionDef,

    // symbol references
    FeatureRef, ScalarRef, VectorRef, DatumRef, SelectionRef
};

struct ScriptToken
{
    SyntaxElementPos range;
    TokenKind kind;
};

/**
 * a range of the script, which can be collapsed in an editor:
 * the name and argument list of a command or an in-situ submodel block.
 */
struct FoldRange
{
    SyntaxElementPos range;
    /**
     * name of the command, empty for an in-situ submodel block
     */
    std::string command;
};



}
}
}

#endif // INSIGHT_CAD_PARSER_TOKENS_H
