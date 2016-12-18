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

#ifndef INSIGHT_LATEXTOOLS_H
#define INSIGHT_LATEXTOOLS_H

#include <string>
#include "base/boost_include.h"

namespace insight 
{

std::string cleanSymbols(const std::string& s);
boost::filesystem::path cleanLatexImageFileName(const boost::filesystem::path& s);

/**
 * Class for handling simplified LaTeX code. 
 * Intended for unified treatment in Qt help and result reports with formulas.
 * Difference to real LaTeX: 
 * - no comments
 * - Special chars "% # _ [ ]" allowed in text (shall be compatible with paths, common filenames, variable names etc)
 * - \\n means line break (equal to double backslash)
 * Only simple subset of latex supported:
 * - inline formula $..$ and unnumbered display formula $$ .. $$
 * - \\includegraphics
 * - \\url
 * Handling includes:
 * - storing
 * - correcting external file references (to files in insight shared directory)
 * - compiling into PDF
 * - converting into HTML
 * - converting into plain text
 */
class SimpleLatex
{
  std::string simpleLatex_code_;
  
public:
  SimpleLatex();
  SimpleLatex(const std::string& simpleLatex);
  
  const std::string& simpleLatex() const;
  std::string& simpleLatex();
  
  std::string toLaTeX() const;
  std::string toHTML(double maxWidth) const;
  std::string toPlainText() const;
  
  static std::string LaTeXFromPlainText(const std::string& plainText);
};

}

#endif // INSIGHT_LATEXTOOLS_H
