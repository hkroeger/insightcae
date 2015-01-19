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

#include <boost/concept_check.hpp>

#include "base/linearalgebra.h"
#include "base/analysis.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "boost/format.hpp"

using namespace std;
using namespace insight;
using namespace boost;

int main(int argc, char *argv[])
{
  insight::GSLExceptionHandling gsl_errtreatment;
  
  using namespace rapidxml;
  namespace po = boost::program_options;
  
  typedef std::vector<string> StringList;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")
      ("workdir,w", po::value<std::string>(), "execution directory")
      ("bool,b", po::value<StringList>(), "boolean variable assignment")
      ("selection,l", po::value<StringList>(), "selection variable assignment")
      ("string,s", po::value<StringList>(), "string variable assignment")
      ("path,p", po::value<StringList>(), "path variable assignment")
      ("double,d", po::value<StringList>(), "double variable assignment")
      ("int,i", po::value<StringList>(), "int variable assignment")
      ("merge,m", po::value<StringList>(), "additional input file to merge into analysis parameters before variable assignments")
      ("input-file,f", po::value< StringList >(),"Specifies input file.")
  ;  
  
  po::positional_options_description p;
  p.add("input-file", -1);  
  
  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
	    options(desc).positional(p).run(), vm);
  po::notify(vm);
  
  if (vm.count("help"))
  {
    cout << desc << endl;
    exit(-1);
  }

  if (!vm.count("input-file"))
  {
    cout<<"input file has to be specified!"<<endl;
    exit(-1);
  }

  try
  {
    std::string fn = vm["input-file"].as<StringList>()[0];
    
    std::string contents;
    try
    {
      std::ifstream in(fn.c_str());
      in.seekg(0, std::ios::end);
      contents.resize(in.tellg());
      in.seekg(0, std::ios::beg);
      in.read(&contents[0], contents.size());
      in.close();
    }
    catch (...)
    {
      throw insight::Exception("Could not open file: "+fn);
    }

    xml_document<> doc;
    doc.parse<0>(&contents[0]);
    
    xml_node<> *rootnode = doc.first_node("root");
    
    std::string analysisName;
    xml_node<> *analysisnamenode = rootnode->first_node("analysis");
    if (analysisnamenode)
    {
      analysisName = analysisnamenode->first_attribute("name")->value();
    }
    /*
    insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_.find(analysisName);
    if (i==insight::Analysis::factories_.end())
      throw insight::Exception("Could not lookup analysis type "+analysisName);
    
    AnalysisPtr analysis( (*i->second)( insight::NoParameters() ) );
    */
    AnalysisPtr analysis ( insight::Analysis::lookup(analysisName, insight::NoParameters()) );
    analysis->setDefaults();
    
    boost::filesystem::path dir = boost::filesystem::absolute(boost::filesystem::path(fn)).parent_path();
    if (vm.count("workdir"))
    {
      dir=boost::filesystem::absolute(vm["workdir"].as<std::string>());
    }
    std::string filestem = boost::filesystem::path(fn).stem().string();
    cout<< "Executing analysis in directory "<<dir<<endl;
    analysis->setExecutionPath(dir);

    ParameterSet parameters = analysis->defaultParameters();
    parameters.readFromNode(doc, *rootnode, dir);
    
    if (vm.count("merge"))
    {
      StringList ists=vm["merge"].as<StringList>();
      BOOST_FOREACH(const string& ist, ists)
      {
	ParameterSet to_merge;
	parameters.readFromFile(ist);
      }
    }
    
    if (vm.count("bool"))
    {
      StringList sets=vm["bool"].as<StringList>();
      BOOST_FOREACH(const string& s, sets)
      {
	std::vector<std::string> pair;
	boost::split(pair, s, boost::is_any_of(":"));
	bool v=boost::lexical_cast<bool>(pair[1]);
	cout << "Setting boolean '"<<pair[0]<<"' = "<<v<<endl;
	parameters.getBool(pair[0])=v;
      }
    }
    
    if (vm.count("string"))
    {
      StringList sets=vm["string"].as<StringList>();
      BOOST_FOREACH(const string& s, sets)
      {
	std::vector<std::string> pair;
	boost::split(pair, s, boost::is_any_of(":"));
	cout << "Setting string '"<<pair[0]<<"' = \""<<pair[1]<<"\""<<endl;
	parameters.getString(pair[0])=pair[1];
      }
    }
    
    if (vm.count("selection"))
    {
      StringList sets=vm["selection"].as<StringList>();
      BOOST_FOREACH(const string& s, sets)
      {
	std::vector<std::string> pair;
	boost::split(pair, s, boost::is_any_of(":"));
	cout << "Setting selection '"<<pair[0]<<"' = \""<<pair[1]<<"\""<<endl;
	parameters.get<SelectionParameter>(pair[0]).setSelection(pair[1]);
      }
    }
    
    if (vm.count("path"))
    {
      StringList sets=vm["path"].as<StringList>();
      BOOST_FOREACH(const string& s, sets)
      {
	std::vector<std::string> pair;
	boost::split(pair, s, boost::is_any_of(":"));
	cout << "Setting path '"<<pair[0]<<"' = \""<<pair[1]<<"\""<<endl;
	parameters.getPath(pair[0])=pair[1];
      }
    }
    
    if (vm.count("double"))
    {
      StringList sets=vm["double"].as<StringList>();
      BOOST_FOREACH(const string& s, sets)
      {
	std::vector<std::string> pair;
	boost::split(pair, s, boost::is_any_of(":"));
	double v=boost::lexical_cast<double>(pair[1]);
	cout << "Setting double '"<<pair[0]<<"' = "<<v<<endl;
	parameters.getDouble(pair[0])=v;
      }
    }

    if (vm.count("int"))
    {
      StringList sets=vm["int"].as<StringList>();
      BOOST_FOREACH(const string& s, sets)
      {
	std::vector<std::string> pair;
	boost::split(pair, s, boost::is_any_of(":"));
	int v=boost::lexical_cast<int>(pair[1]);
	cout << "Setting int '"<<pair[0]<<"' = "<<v<<endl;
	parameters.getInt(pair[0])=v;
      }
    }

    analysis->setParameters(parameters);
    
    // run analysis
    TextProgressDisplayer pd;
    ResultSetPtr results = (*analysis)(&pd);

    boost::filesystem::path outpath=analysis->executionPath()/ (filestem+".tex");
    results->writeLatexFile( outpath );
    
    if ( ::system( str( format("cd %s && pdflatex \"%s\"") % dir.string() % outpath.string() ).c_str() ))
    {
      Warning("TeX input file was written but could not execute pdflatex successfully.");
    }
  }
  catch (insight::Exception e)
  {
    cout<<"Exception occured: "<<e<<endl;
  }

  return 0;
}