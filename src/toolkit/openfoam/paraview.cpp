
#include "openfoam/paraview.h"

#include "openfoam/ofes.h"

#include "boost/assign.hpp"
#include "boost/algorithm/string.hpp"

#include <cmath>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{


namespace paraview
{




std::string pvec(const arma::mat& v)
{
  return boost::str( boost::format("np.array([%g, %g, %g])") % v(0) % v(1) % v(2) );
}




defineType(PVScriptElement);
defineDynamicClass(PVScriptElement);

PVScriptElement::PVScriptElement(const Parameters &p, const std::vector<std::string>& objNames)
  : objNames_(objNames), p_(p)
{}

PVScriptElement::~PVScriptElement()
{}



std::vector<boost::filesystem::path> PVScriptElement::createdFiles() const
{
  return std::vector<boost::filesystem::path>();
}

std::vector<std::string> PVScriptElement::createdObjects() const
{
  return objNames_;
}




defineType(CustomScriptElement);
addToFactoryTable(PVScriptElement, CustomScriptElement);
addToStaticFunctionTable(PVScriptElement, CustomScriptElement, defaultParameters);

CustomScriptElement::CustomScriptElement(const ParameterSet& ps)
  : PVScriptElement(ps, Parameters(ps).names), p_(ps)
{}


string CustomScriptElement::pythonCommands() const
{
  return p_.command;
}




defineType(Arrows);
addToFactoryTable(PVScriptElement, Arrows);
addToStaticFunctionTable(PVScriptElement, Arrows, defaultParameters);

Arrows::Arrows(const ParameterSet& ps)
: PVScriptElement(ps, {Parameters(ps).name}), p_(ps)
{}


string Arrows::pythonCommands() const
{
  std::string pycmd =
      p_.name+"_input=[]\n"
      ;
  for (const auto& a: p_.arrows)
  {
    arma::mat R = a.to - a.from;
    double r=norm(R,2);
    R/=r;
    double beta = 180.*atan2( -R(2), sqrt(1.-pow(R(2),2))) / M_PI;
    double gamma = 180.*std::asin(R(1)/sqrt(1.-pow(R(2),2)))/M_PI;

    pycmd +=
        p_.name+"_source = Arrow()\n"+
        p_.name+"_t1 = Transform(Input="+p_.name+"_source)\n"+
        p_.name+"_t1.Transform = 'Transform'\n"+
        p_.name+"_t1.Transform.Scale = "+boost::str(boost::format("[%g, %g, %g]\n")%r%r%r)+
        p_.name+"_t1.Transform.Rotate = "+boost::str(boost::format("[0.0, %g, %g]\n")%beta%gamma)+
        p_.name+"_obj = Transform(Input="+p_.name+"_t1)\n"+
        p_.name+"_obj.Transform = 'Transform'\n"+
        p_.name+"_obj.Transform.Translate = "+boost::str(boost::format("[%g, %g, %g]\n")%a.from(0)%a.from(1)%a.from(2))+
        p_.name+"_input.append("+p_.name+"_obj)\n"
        ;
  }

  pycmd +=
      p_.name+" = GroupDatasets(Input = "+p_.name+"_input)\n"
      ;

  return pycmd;
}



defineType(Cutplane);
addToFactoryTable(PVScriptElement, Cutplane);
addToStaticFunctionTable(PVScriptElement, Cutplane, defaultParameters);

Cutplane::Cutplane(const ParameterSet& ps)
: PVScriptElement(ps, {Parameters(ps).name}), p_(ps)
{}


string Cutplane::pythonCommands() const
{
  return
      p_.name + " = planarSlice("+p_.dataset+", "+pvec(p_.p0)+","+pvec(p_.normal)+")\n"
      "displayContour("+p_.name+", '"+p_.field+"', arrayType='CELL_DATA', barpos=[0.7,0.25], barorient=1)\n"
      ;
}




defineType(Streamtracer);
addToFactoryTable(PVScriptElement, Streamtracer);
addToStaticFunctionTable(PVScriptElement, Streamtracer, defaultParameters);

Streamtracer::Streamtracer(const ParameterSet& ps)
: PVScriptElement(ps, {Parameters(ps).name}), p_(ps)
{}


string Streamtracer::pythonCommands() const
{
  std::string cmd;
  cmd +=
      p_.name+"=StreamTracer(Input="+p_.dataset+", "+
              "Vectors=['"+p_.field+"'], "+
              "MaximumStreamlineLength="+lexical_cast<string>(p_.maxLen)+")\n";

  if (const auto* cloud = boost::get<Parameters::seed_cloud_type>(&p_.seed))
  {
    cmd+=
       p_.name+".SeedType = 'Point Source'\n"+
       p_.name+".SeedType.Center="+pvec(cloud->center)+"\n"+
       p_.name+".SeedType.Radius="+lexical_cast<string>(cloud->radius)+"\n"+
       p_.name+".SeedType.NumberOfPoints="+lexical_cast<string>(cloud->number)+"\n"
       ;
  }
  return cmd;
}






defineType(PVScene);
defineDynamicClass(PVScene);

PVScene::PVScene(const Parameters& p)
  : p_(p)
{}

PVScene::~PVScene()
{}

std::vector<std::string> PVScene::objectNames() const
{
  std::vector<std::string> objnames;
  for (PVScriptElementPtr scn: p_.sceneElements)
  {
    auto sn=scn->createdObjects();
    std::copy(sn.begin(), sn.end(), std::back_inserter(objnames));
  }
  return objnames;
}


std::string PVScene::sceneObjectsBoundingBoxCmd(const std::vector<std::string> &vn) const
{
  auto objnames = objectNames();
  return
      vn[0]+"="+vn[1]+"="+vn[2]+"=1e10\n"+
      vn[3]+"="+vn[4]+"="+vn[5]+"=-1e10\n"+
      "for o in ["+boost::algorithm::join(objnames, ",")+"]:\n"
      " (xmin,xmax,ymin,ymax,zmin,zmax)=o.GetDataInformation().GetBounds()\n"
      " "+vn[0]+"=min(xmin,"+vn[0]+")\n"
      " "+vn[1]+"=min(ymin,"+vn[1]+")\n"
      " "+vn[2]+"=min(zmin,"+vn[2]+")\n"
      " "+vn[3]+"=max(xmax,"+vn[3]+")\n"
      " "+vn[4]+"=max(ymax,"+vn[4]+")\n"
      " "+vn[5]+"=max(zmax,"+vn[5]+")\n"
      ;
}

string PVScene::pythonCommands() const
{
  std::string pvscript;

  for (PVScriptElementPtr scn: p_.sceneElements)
  {
    pvscript += scn->pythonCommands();
  }

  auto ons=objectNames();
  for (const auto& on: ons)
  {
    pvscript += "Show("+on+")\n";
  }

  return pvscript;
}


std::vector<boost::filesystem::path> PVScene::createdFiles() const
{
  return std::vector<boost::filesystem::path>({p_.imagename});
}




defineType(SingleView);
addToFactoryTable(PVScene, SingleView);
addToStaticFunctionTable(PVScene, SingleView, defaultParameters);

SingleView::SingleView(const ParameterSet& ps)
: PVScene(ps), p_(ps)
{}


string SingleView::pythonCommands() const
{
  arma::mat e_up = p_.e_up; e_up/=norm(e_up, 2);
  arma::mat e_n = -p_.normal; e_n/=norm(e_n, 2);
  arma::mat eq=arma::cross(e_up, e_n);
  eq/=norm(eq,2);

  return
      PVScene::pythonCommands()
      +
      sceneObjectsBoundingBoxCmd({"xmi", "ymi", "zmi", "xma", "yma", "zma"})
      +
      "L=np.array([xma-xmi,yma-ymi,zma-zmi])\n"
      +
      "setCam("
        + pvec(p_.lookAt-p_.normal) + ", "
        + pvec(p_.lookAt) + ", "
        + pvec(p_.e_up) + ", "
        + "(abs(np.dot(L,"+pvec(eq)+")),abs(np.dot(L,"+pvec(e_up)+")))"
      +")\n"
      "WriteImage('"+p_.imagename+".png')\n"
  ;
}

std::vector<boost::filesystem::path> SingleView::createdFiles() const
{
  return std::vector<boost::filesystem::path>
      ({ p_.imagename+".png" })
      ;
}




defineType(IsoView);
addToFactoryTable(PVScene, IsoView);
addToStaticFunctionTable(PVScene, IsoView, defaultParameters);

IsoView::IsoView(const ParameterSet& ps)
: PVScene(ps), p_(ps)
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
      PVScene::pythonCommands()
      +
      "setCam("
        + pvec(ctr+ex*Lx*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
        + str(format("(%g,%g)") % Ly % Lz)
      +")\n"
      "WriteImage('"+p_.imagename+"_front.png')\n"
      +
      "setCam("
        + pvec(ctr+ez*Lz*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(-ey) + ", "
        + str(format("(%g,%g)") % Lx % Ly)
      +")\n"
      "WriteImage('"+p_.imagename+"_top.png')\n"
      +
      "setCam("
        + pvec(ctr+ey*Ly*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
        + str(format("(%g,%g)") % Lx % Lz)
      +")\n"
      "WriteImage('"+p_.imagename+"_side.png')\n"
      +
      "setCam("
        + pvec(ctr+ (ex*Lx +ey*Ly +ez*Lz)*1.5) + ", "
        + pvec(ctr) + ", "
        + pvec(ez) + ", "
        + str(format("%g") % (0.5*sqrt(Lx*Lx+Ly*Ly+Lz*Lz)))
      +")\n"
      "WriteImage('"+p_.imagename+"_diag.png')\n"
  ;
}

std::vector<boost::filesystem::path> IsoView::createdFiles() const
{
  return std::vector<boost::filesystem::path>({
       p_.imagename+"_front.png",
       p_.imagename+"_top.png",
       p_.imagename+"_side.png",
       p_.imagename+"_diag.png"
      });
}




addToAnalysisFactoryTable(ParaviewVisualization);

ParaviewVisualization::ParaviewVisualization(const ParameterSet& ps, const boost::filesystem::path& exepath)
: Analysis("", "", ps, exepath)
{
}

ResultSetPtr ParaviewVisualization::operator()(ProgressDisplayer&)
{
//  boost::mutex::scoped_lock lock(runPvPython_mtx);
    setupExecutionEnvironment();

    Parameters p(parameters_);

    std::string pvscript=
        "from Insight.Paraview import *\n"
        "import numpy as np\n"+
        OFCaseDatasetName() + " = loadOFCase('.')\n"
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
      tf << pvscript;
      tf.close();
    }
    args.push_back(tempfile.c_str());
    if (OFEs::list.size()==0)
      throw insight::Exception("no OpenFOAM environment defined!");

    OpenFOAMCase ofc( *OFEs::list.begin()->second );
    ofc.executeCommand(executionPath(), "pvbatch", args, nullptr, 0, &machine);

//    if (!keepScript)
//      remove(tempfile);

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

string ParaviewVisualization::OFCaseDatasetName()
{
  return "openfoam_case";
}


}



}
