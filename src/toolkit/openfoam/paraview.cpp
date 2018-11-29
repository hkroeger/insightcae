
#include "paraview.h"

#include "boost/assign.hpp"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{


namespace paraview
{

defineType(PVScene);
// defineFactoryTableNoArgs(PVScene);
defineDynamicClass(PVScene);

PVScene::~PVScene()
{}

std::string PVScene::pvec(const arma::mat& v)
{
  return boost::str( boost::format("[%g, %g, %g]") % v(0) % v(1) % v(2) );
}

std::vector<boost::filesystem::path> PVScene::createdFiles() const
{
  return std::vector<boost::filesystem::path>();
}

defineType(CustomPVScene);
addToFactoryTable(PVScene, CustomPVScene);
addToStaticFunctionTable(PVScene, CustomPVScene, defaultParameters);

CustomPVScene::CustomPVScene(const ParameterSet& ps)
: p_(ps)
{}


string CustomPVScene::pythonCommands() const
{
  return p_.command;
}

defineType(Cutplane);
addToFactoryTable(PVScene, Cutplane);
addToStaticFunctionTable(PVScene, Cutplane, defaultParameters);

Cutplane::Cutplane(const ParameterSet& ps)
: p_(ps)
{}


string Cutplane::pythonCommands() const
{
  return "";
}

defineType(IsoView);
addToFactoryTable(PVScene, IsoView);
addToStaticFunctionTable(PVScene, IsoView, defaultParameters);

IsoView::IsoView(const ParameterSet& ps)
: p_(ps)
{}


string IsoView::pythonCommands() const
{
  arma::mat bbL = p_.bbmax - p_.bbmin;
  arma::mat ctr = p_.bbmin + 0.5*bbL;
  arma::mat ex, ey, ez;
  ex=ey=ez=vec3(0,0,0);

  arma::uvec indices = arma::sort_index(bbL, "descend");
  ex(indices(0))=1.0;
  ez(indices(2))=1.0;
  ey=arma::cross(ez, ex);
  arma::mat L = bbL(indices);

  return
      "setCam("
        + pvec(ctr+ex*L[0]*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
        + str(format("%g") % (0.5*std::max(L[1],L[2]))) + (L[1]>L[2]?", scaleIsHorizontal=True":"")
      +")\n"
      "WriteImage('"+p_.filename.filename().stem().string()+"_front"+p_.filename.extension().string()+"')\n"
      +
      "setCam("
        + pvec(ctr+ez*L[2]*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(-ey) + ", "
      + str(format("%g") % (0.5*std::max(L[1],L[0]))) + (L[0]>L[2]?", scaleIsHorizontal=True":"")
      +")\n"
      "WriteImage('"+p_.filename.filename().stem().string()+"_top"+p_.filename.extension().string()+"')\n"
      +
      "setCam("
        + pvec(ctr+ey*L[1]*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
      + str(format("%g") % (0.5*std::max(L[0],L[2]))) + (L[0]>L[2]?", scaleIsHorizontal=True":"")
      +")\n"
      "WriteImage('"+p_.filename.filename().stem().string()+"_side"+p_.filename.extension().string()+"')\n"
      +
      "setCam("
        + pvec(ctr+ex*L[0]*1.5+ey*L[1]*1.5+ez*L[2]*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
      + str(format("%g") % (0.5*L[0])) + ", scaleIsHorizontal=True"
      +")\n"
      "WriteImage('"+p_.filename.filename().stem().string()+"_diag"+p_.filename.extension().string()+"')\n"
  ;
}

std::vector<boost::filesystem::path> IsoView::createdFiles() const
{
  return list_of<boost::filesystem::path>
      ( p_.filename.filename().stem().string()+"_front"+p_.filename.extension().string() )
      ( p_.filename.filename().stem().string()+"_top"+p_.filename.extension().string() )
      ( p_.filename.filename().stem().string()+"_side"+p_.filename.extension().string() )
      ( p_.filename.filename().stem().string()+"_diag"+p_.filename.extension().string() )
      ;
}




addToAnalysisFactoryTable(ParaviewVisualization);

ParaviewVisualization::ParaviewVisualization(const ParameterSet& ps, const boost::filesystem::path& exepath)
: Analysis("", "", ps, exepath)
{
}

ParameterSet ParaviewVisualization::defaultParameters()
{
  return Parameters::makeDefault();
}

ResultSetPtr ParaviewVisualization::operator()(ProgressDisplayer*)
{
//  boost::mutex::scoped_lock lock(runPvPython_mtx);
    setupExecutionEnvironment();

    Parameters p(parameters_);

    std::string pvscript=
     "openfoam_case=loadOFCase('.')\n"
     "prepareSnapshots()\n"
    ;

    for (PVScenePtr scn: p.scenes)
    {
        pvscript += scn->pythonCommands();
    }


    redi::opstream proc;
    std::vector<string> args;
    args.push_back("--use-offscreen-rendering");
    std::string machine=""; // execute always on local machine
    //ofc.forkCommand(proc, location, "pvpython", args);

    path tempfile=absolute(unique_path("%%%%%%%%%.py"));
    {
      std::ofstream tf(tempfile.c_str());
      tf << "from Insight.Paraview import *\n";
      tf << pvscript;
      tf.close();
    }
    args.push_back(tempfile.c_str());
    if (OFEs::list.size()==0)
      throw insight::Exception("no OpenFOAM environment defined!");

    OpenFOAMCase ofc( *OFEs::list.begin()->second );
    ofc.executeCommand(executionPath(), "pvbatch", args, NULL, 0, &machine);

//    if (!keepScript)
      remove(tempfile);

    ResultSetPtr results(new ResultSet(parameters_, "Paraview renderings", ""));

    std::vector<boost::filesystem::path> files;
    for (PVScenePtr scn: p.scenes)
    {
        std::vector<boost::filesystem::path> af=scn->createdFiles();
        std::copy(af.begin(), af.end(), std::back_inserter(files));
    }

    for (boost::filesystem::path f: files)
    {
      results->insert(f.filename().stem().string(),
        std::auto_ptr<Image>(new Image
        (
        executionPath(), f,
        f.filename().stem().string(), ""
      )));
    }

    return results;
}


}



}
