#include <boost/concept_check.hpp>

#include "base/analysis.h"

#include <fstream>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

using namespace insight;

int main(int argc, char *argv[])
{
  using namespace rapidxml;
  
  std::string fn(argv[1]);
  
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
  
  insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_.find(analysisName);
  if (i==insight::Analysis::factories_.end())
    throw insight::Exception("Could not lookup analysis type "+analysisName);
  
  AnalysisPtr analysis( (*i->second)( insight::NoParameters() ) );
  analysis->setDefaults();
  ParameterSetPtr parameters ( analysis->defaultParameters().clone() );

  parameters->readFromNode(doc, *rootnode);
  
  boost::filesystem::path dir=boost::filesystem::path(fn).parent_path();
  analysis->setExecutionPath(dir);

  // run analysis
  TextProgressDisplayer pd;
  ResultSetPtr results = (*analysis)(&pd);

  boost::filesystem::path outpath=analysis->executionPath()/"report.tex";
  results->writeLatexFile( outpath );

  return 0;
}