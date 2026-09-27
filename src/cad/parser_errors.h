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

#ifndef INSIGHT_CAD_PARSER_ERRORS_H
#define INSIGHT_CAD_PARSER_ERRORS_H

#include <set>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/spirit/home/support/info.hpp"

namespace insight {
namespace cad {
namespace parser {


/**
 * helper functions for generating understandable
 * error messages from ISCAD parser failures
 */

struct SourceLocation
{
    int line=0, column=0; // both 1-based, 0 if unknown
    std::string lineText;
};

SourceLocation locateInSource(const std::string& text, std::size_t offset);

/**
 * advance offset over whitespace and comments
 * (same syntax as skip_grammar)
 */
std::size_t skipWhitespaceAndComments(const std::string& text, std::size_t offset);

/**
 * length of the token starting at offset
 * (identifier, number, quoted string or single character)
 */
std::size_t tokenLength(const std::string& text, std::size_t offset);

/**
 * the identifier starting at offset or an empty string,
 * if there is none
 */
std::string identifierAt(const std::string& text, std::size_t offset);

/**
 * human readable description of the token at offset,
 * e.g. "';'" or "end of input"
 */
std::string describeToken(const std::string& text, std::size_t offset);

/**
 * readable names of the alternatives, which the failed
 * parser would have accepted. Literals are single-quoted,
 * e.g. {"','", "')'", "scalar expression"}
 */
std::vector<std::string> expectedAlternatives(const boost::spirit::info& what);

/**
 * joins alternatives into e.g. "',', ')' or scalar expression"
 */
std::string describeAlternatives(std::vector<std::string> alternatives);

/**
 * converts the description of a failed parser
 * into a readable list of alternatives, e.g. "',' or ')'"
 */
std::string describeExpected(const boost::spirit::info& what);

/**
 * returns the names from candidates, which are similar to name
 * (small edit distance), best match first
 */
std::vector<std::string> similarNames(
    const std::string& name,
    const std::set<std::string>& candidates,
    std::size_t maxSuggestions = 3 );

/**
 * "Did you mean 'a' or 'b'?" or empty string
 */
std::string didYouMean(const std::vector<std::string>& suggestions);

/**
 * checks, if the statement at offset is probably
 * missing its terminating semicolon and returns a hint.
 * Returns an empty string otherwise.
 */
std::string missingSemicolonHint(const std::string& text, std::size_t offset);

/**
 * compiler style error message:
 *
 * file:line:col: error: message
 *    line | source text
 *         |      ^~~~
 *   note
 */
std::string formatDiagnostic(
    const boost::filesystem::path& file,
    const std::string& text,
    int from_pos, int to_pos,
    const std::string& message,
    const std::vector<std::string>& notes );


} // namespace parser
} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_PARSER_ERRORS_H
