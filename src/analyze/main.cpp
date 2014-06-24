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

using namespace std;
using namespace insight;

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
      ("bool,b", po::value<StringList>(), "boolean variable assignment")
      ("merge,m", po::value<StringList>(), "additional input file to merge into analysis parameters before variable assignments")
      ("input-file,i", po::value< StringList >(),"Specifies input file.")
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
    
    std::ifstream in(fn.c_str());
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

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
	cout << "Setting '"<<pair[0]<<"' = "<<v<<endl;
	parameters.getBool(pair[0])=v;
      }
    }
    
    analysis->setParameters(parameters);
    
    // run analysis
    TextProgressDisplayer pd;
    ResultSetPtr results = (*analysis)(&pd);

    boost::filesystem::path outpath=analysis->executionPath()/ (filestem+".tex");
    results->writeLatexFile( outpath );
  }
  catch (insight::Exception e)
  {
    cout<<"Exception occured: "<<e<<endl;
  }

  return 0;
}