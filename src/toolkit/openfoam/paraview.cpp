
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


defineType(Streamtracer);
addToFactoryTable(PVScene, Streamtracer);
addToStaticFunctionTable(PVScene, Streamtracer, defaultParameters);

Streamtracer::Streamtracer(const ParameterSet& ps)
: p_(ps)
{}


string Streamtracer::pythonCommands() const
{
  std::string cmd;
  cmd+=p_.name+"=StreamTracer(Input="+p_.dataset+", Vectors=['"+p_.field+"'], MaximumStreamlineLength="+lexical_cast<string>(p_.maxLen)+")\n";
  if (const auto* cloud = boost::get<Parameters::seed_cloud_type>(&p_.seed))
  {
    cmd+=
       p_.name+".SeedType = 'Point Source'\n"+
       p_.name+".SeedType.Center="+paraview::PVScene::pvec(cloud->center)+"\n"+
       p_.name+".SeedType.Radius="+lexical_cast<string>(cloud->radius)+"\n"
       ;
  }

  cmd+="Show("+p_.name+")\n";

  return cmd;
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
  std::cout<<"bbL="<<bbL<<std::endl;
  arma::mat ctr = p_.bbmin + 0.5*bbL;

  arma::mat ez = p_.e_up / arma::norm(p_.e_up, 2);
  arma::mat ex = arma::cross(ez, arma::cross(p_.e_ax, ez));
  ex /= arma::norm(ex, 2);
  arma::mat ey = arma::cross(ez, ex);
  ey /= arma::norm(ey, 2);

  double
      Lx = fabs(arma::dot(bbL, ex)),
      Ly = fabs(arma::dot(bbL, ey)),
      Lz = fabs(arma::dot(bbL, ez))
           ;

  return
      "setCam("
        + pvec(ctr+ex*Lx*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
        + str(format("(%g,%g)") % Ly % Lz)
      +")\n"
      "WriteImage('"+p_.filename.filename().stem().string()+"_front"+p_.filename.extension().string()+"')\n"
      +
      "setCam("
        + pvec(ctr+ez*Lz*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(-ey) + ", "
        + str(format("(%g,%g)") % Lx % Ly)
      +")\n"
      "WriteImage('"+p_.filename.filename().stem().string()+"_top"+p_.filename.extension().string()+"')\n"
      +
      "setCam("
        + pvec(ctr+ey*Ly*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
        + str(format("(%g,%g)") % Lx % Lz)
      +")\n"
      "WriteImage('"+p_.filename.filename().stem().string()+"_side"+p_.filename.extension().string()+"')\n"
      +
      "setCam("
        + pvec(ctr+ (ex*Lx +ey*Ly +ez*Lz)*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
        + str(format("%g") % (0.5*sqrt(Lx*Lx+Ly*Ly+Lz*Lz)))
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
    ofc.executeCommand(executionPath(), "pvbatch", args, nullptr, 0, &machine);

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
        std::unique_ptr<Image>(new Image
        (
        executionPath(), f,
        f.filename().stem().string(), ""
      )));
    }

    return results;
}


}



}
