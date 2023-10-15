
#include "base/boost_include.h"

#include "detectionhandler.h"
#include "analyzeserverdetector.h"

using namespace std;
using namespace insight;
using namespace boost;

int main()
{
  string analysisName1 = "Test Analysis Name 1";
  string analysisName2 = "Test Analysis Name 2";

  auto dh1 = DetectionHandler::start(
                          8090, "127.0.0.1", 8090, analysisName1
                          );

  auto dh2 = DetectionHandler::start(
                          8090, "127.0.0.1", 8092, analysisName2
                          );


  auto di = AnalyzeServerDetector::detectInstances(1000);

  for (const auto& ai: di)
  {
    cout<<ai.serverAddress<<", "<<ai.serverPort<<", "<<ai.analysisName<<endl;
  }

  return di.size()==2 ? 0 : -1;
}
