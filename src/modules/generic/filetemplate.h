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
#ifndef INSIGHT_FILETEMPLATE_H
#define INSIGHT_FILETEMPLATE_H

#include "base/analysis.h"

namespace insight
{
  
class FileTemplate
: public insight::Analysis
{
  
public:
  
#include "filetemplate__FileTemplate__Parameters.h"

/*
PARAMETERSET>>> FileTemplate Parameters

template_archive = path "" "Path to archive which contains all the template files"

numerical = array [ set {
 name = string "1" "Identifier of the variable."
 value = double 0.0 "Value for replacement"
} ] *0 "Numerical variables in file template"

<<<PARAMETERSET
*/

protected:
    // derived data
  
public:
    declareType("FileTemplate");
    FileTemplate(const NoParameters&);
    
    virtual ParameterSet defaultParameters() const;
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);
    
};
}
#endif // INSIGHT_FILETEMPLATE_H
