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
#include "filetemplate.h"
#include "base/softwareenvironment.h"
#include "base/factory.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight {

defineType(FileTemplate);
addToFactoryTable(Analysis, FileTemplate, NoParameters);

FileTemplate::FileTemplate(const NoParameters&)
: Analysis("FileTemplate", "File template based analysis")
{
}

ParameterSet FileTemplate::defaultParameters() const
{
    return Parameters::makeDefault();
}

ResultSetPtr FileTemplate::operator()(ProgressDisplayer* displayer)
{
    Parameters p(*parameters_);
    
    path dir = setupExecutionEnvironment();
    SoftwareEnvironment g;
    
    g.executeCommand(
      str(format("cd %s; tar xzf %s") % absolute(dir).string() % absolute(p.template_archive).string() )
    );
    
    ResultSetPtr results(new ResultSet(parameters(), name_, "Result Report"));


    return results;
}

}
