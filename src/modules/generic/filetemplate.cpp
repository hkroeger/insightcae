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
    
    // unpack files
    std::vector<std::string> filelist;
    g.executeCommand(
      str(format("cd %s; tar vxzf %s") % absolute(dir).string() % absolute(p.template_archive).string() ),
      std::vector<std::string>(),
      &filelist
    );
    
    // ===== replace keyword occurences
    
    // build replacement cmd for sed
    std::string replacecmd;
    BOOST_FOREACH(Parameters::numerical_default_type& ne, p.numerical)
    {
      if (replacecmd.size()>0) replacecmd+=";";
      
      replacecmd+=str(format("s/###%s###/%g/g")
        %ne.name%ne.value);
    }
    std::cout<<"replacecmd="<<replacecmd<<std::endl;
    
    // apply sed to all files except scripts
    BOOST_FOREACH(std::string fn, filelist)
    {
      if (boost::filesystem::is_regular_file(dir/fn))
      {
	if ((fn!=ReservedFileNames[RUNSCRIPT])&&(fn!=ReservedFileNames[EVALSCRIPT]))
	{
	  std::cout<<"replacing variables in file "<<fn<<std::endl;
	  g.executeCommand(
	    str(format("cd %s; sed -ie '%s' %s") 
	      % absolute(dir).string()
	      % replacecmd
	      % fn )
	  );
	}
      }
    }
    
    // ==== execute external analysis command
    path rscr=dir/ReservedFileNames[RUNSCRIPT];
    if (boost::filesystem::exists(rscr))
    {
      std::cout<<"executing analysis run script (assuming it is executable)"<<rscr<<std::endl;
      g.executeCommand(
	str(format("cd %s; ./%s") 
	  % absolute(dir).string()
	  % rscr.filename().string()
       )
      );
    }
    
    // ==== return results
    // execute eval script
    path escr=dir/ReservedFileNames[EVALSCRIPT];
    if (boost::filesystem::exists(escr))
    {
      std::cout<<"executing evaluation run script (assuming it is executable)"<<escr<<std::endl;
      g.executeCommand(
	str(format("cd %s; ./%s") 
	  % absolute(dir).string()
	  % escr.filename().string()
       )
      );
    }

    // read in the results
    ResultSetPtr results(new ResultSet(parameters(), name_, "Result Report"));
    path resf=dir/ReservedFileNames[EVALRESULTS];
    if (boost::filesystem::exists(resf))
    {
      results->readFromFile(resf);
    }
    else
    {
      results->insert(
	"remark", 
	new Comment(
	  "The run script did not return result information by creating a file ``INSIGHT\\_RESULTS.isr''", 
	  "", 
	  ""
	)
      );

    }
    
    return results;
}

}
