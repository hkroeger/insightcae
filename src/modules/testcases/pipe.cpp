/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "pipe.h"

#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/basiccaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace arma;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{
  
defineType(PipeBase);

PipeBase::PipeBase(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Pipe Flow Test Case",
    "Cylindrical domain with cyclic BCs on axial ends"
  ),
  cycl_in_("cycl_half0"),
  cycl_out_("cycl_half1")
{}

PipeBase::~PipeBase()
{

}

ParameterSet PipeBase::defaultParameters() const
{
  ParameterSet p(OpenFOAMAnalysis::defaultParameters());
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
    
      ("geometry", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("D",		new DoubleParameter(1.0, "[m] Diameter of the pipe"))
	    ("L",		new DoubleParameter(1.0, "[m] Length of the pipe"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the bearing"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nax",	new IntParameter(100, "# cells in axial direction"))
	    ("s",	new DoubleParameter(1.0, "Axial grid anisotropy (ratio of axial cell edge length to lateral edge length)"))
	    ("x",	new DoubleParameter(0.5, "Edge length of core block as fraction of diameter"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("Re_tau",		new DoubleParameter(100, "[-] Friction-Velocity-Reynolds number"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("turbulenceModel",new SelectionParameter(0, turbulenceModel::factoryToC(), "Turbulence model"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}

std::string PipeBase::cyclPrefix() const
{
  boost:smatch m;
  boost::regex_search(cycl_in_, m, boost::regex("(.*)_half[0,1]"));
  std::string namePrefix=m[1];
  cout<<namePrefix<<endl;
  return namePrefix;
}


double PipeBase::calcLc(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "mesh", x);
  return x*D;
}

int PipeBase::calcnc(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", x);
  PSDBL(p, "mesh", s);
  double Delta=L/double(nax);
  return D*(M_PI+4.*x)/(8.*Delta/s);
}

int PipeBase::calcnr(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", s);
  PSDBL(p, "mesh", x);
  double Delta=L/double(nax);
  return D*(1.-sqrt(2.)*x)/(2.*Delta/s);
}

#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>

double lambda_func(double lambda, void *param)
{
  double Retau=*static_cast<double*>(param);
  //cout<<Retau<<" "<<lambda<<endl;
  //cout<<(2./5.)+1./(2.*lambda)<<endl;
  double Re=pow(10, (2./5.)+1./(2.*lambda))/sqrt(lambda);
  return 2.*Retau*sqrt(8./lambda) - Re;
}

double PipeBase::calcRe(const ParameterSet& p) const
{
  PSDBL(p, "operation", Re_tau);
  
  int i, times, status;
  gsl_function f;
  gsl_root_fsolver *workspace_f;
  double x, x_l, x_r;

 
    /* Define Solver */
    workspace_f = gsl_root_fsolver_alloc(gsl_root_fsolver_bisection);
 
    //printf("F solver: %s\n", gsl_root_fsolver_name(workspace_f));
 
    f.function = &lambda_func;
    f.params = &Re_tau;
 
    /* set initial interval */
    x_l = 1e-2;
    x_r = 10;
 
    /* set solver */
    gsl_root_fsolver_set(workspace_f, &f, x_l, x_r);
 
    /* main loop */
    for(times = 0; times < 100; times++)
    {
        status = gsl_root_fsolver_iterate(workspace_f);
 
        x_l = gsl_root_fsolver_x_lower(workspace_f);
        x_r = gsl_root_fsolver_x_upper(workspace_f);
        //printf("%d times: [%10.3e, %10.3e]\n", times, x_l, x_r);
 
        status = gsl_root_test_interval(x_l, x_r, 1.0e-13, 1.0e-20);
        if(status != GSL_CONTINUE)
        {
            //printf("Status: %s\n", gsl_strerror(status));
            //printf("\n Root = [%25.17e, %25.17e]\n\n", x_l, x_r);
            break;
        }
    }
 
    /* free */
    gsl_root_fsolver_free(workspace_f);
    double lambda=x_l;
    double Re=2.*Re_tau*sqrt(8./x_l);
    cout<<"Re="<<Re<<endl;
    return Re;
}

double PipeBase::calcUbulk(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  //PSDBL(p, "operation", Re_tau);
  
  return 1./D; //calcRe(p)*(1./Re_tau)/D;
}

void PipeBase::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  
  double Lc=calcLc(p);
  int nc=calcnc(p);
  int nr=calcnr(p);
  cout<<"Lc="<<Lc<<", nc="<<nc<<", nr="<<nr<<endl;
    
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  
  double al = M_PI/2.;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (11, 	vec3(0, 0.5*D, 0))
      (10, 	vec3(0,  cos(0.5*al)*Lc, 0.))
      (9, 	vec3(0,  1.2*0.5*Lc, 0.))
  ;
  arma::mat vL=vec3(L, 0, 0);
  arma::mat ax=vec3(1, 0, 0);
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  
  {
    arma::mat r0=rotMatrix(0.5*al, ax);
    arma::mat r1=rotMatrix(1.5*al, ax);
    arma::mat r2=rotMatrix(2.5*al, ax);
    arma::mat r3=rotMatrix(3.5*al, ax);

    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  r1*pts[10], r2*pts[10], r3*pts[10], r0*pts[10],
	  (r1*pts[10])+vL, (r2*pts[10])+vL, (r3*pts[10])+vL, (r0*pts[10])+vL
	),
	nc, nc, nax
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));
  }

  for (int i=0; i<4; i++)
  {
    arma::mat r0=rotMatrix(double(i+0.5)*al, ax);
    arma::mat r1=rotMatrix(double(i+1.5)*al, ax);
    
    Block& bl = bmd->addBlock
    (
      new Block(P_8(
	  r1*pts[10], r0*pts[10], r0*pts[11], r1*pts[11],
	  (r1*pts[10])+vL, (r0*pts[10])+vL, (r0*pts[11])+vL, (r1*pts[11])+vL
	),
	nc, nr, nax
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));

    arma::mat rmid=rotMatrix(double(i+1)*al, ax);
    bmd->addEdge(new ArcEdge(r1*pts[11], r0*pts[11], rmid*pts[11]));
    bmd->addEdge(new ArcEdge((r1*pts[11])+vL, (r0*pts[11])+vL, (rmid*pts[11])+vL));

    //inner core
    bmd->addEdge(new ArcEdge(r1*pts[10], r0*pts[10], rmid*pts[9]));
    bmd->addEdge(new ArcEdge((r1*pts[10])+vL, (r0*pts[10])+vL, (rmid*pts[9])+vL));

  }

  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
}


void PipeBase::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
    
  cm.insert(new pimpleFoamNumerics(cm) );
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_fields(list_of<std::string>("p")("U"))
  ));
  cm.insert(new probes(cm, probes::Parameters()
    .set_fields(list_of<std::string>("p")("U"))
    .set_probeLocations(list_of<arma::mat>
      (vec3(0.1*L, 0, 0))
      (vec3(0.33*L, 0, 0))
      (vec3(0.5*L, 0, 0))
      (vec3(0.66*L, 0, 0))
      (vec3(0.9*L, 0, 0))
      (vec3(0.1*L, 0.9*0.5*D, 0))
      (vec3(0.33*L, 0.9*0.5*D, 0))
      (vec3(0.5*L, 0.9*0.5*D, 0))
      (vec3(0.66*L, 0.9*0.5*D, 0))
      (vec3(0.9*L, 0.9*0.5*D, 0))
    )
  ));
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(1./calcRe(p)) ));
  
//   cm.insert(new VelocityInletBC(cm, cycl_in_, boundaryDict, VelocityInletBC::Parameters()
//     .set_velocity(vec3(calcUbulk(p), 0, 0)) 
//   ));
//   cm.insert(new PressureOutletBC(cm, cycl_out_, boundaryDict, PressureOutletBC::Parameters()
//     .set_pressure(0.0) 
//   ));
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());

/*  
  cm.executeCommand(dir, "setVortexVelocity",
    list_of<std::string>
    (lexical_cast<std::string>(Gamma))
    ("("+lexical_cast<std::string>(vcx)+" "+lexical_cast<std::string>(vcy)+" 0)")
  );  
  */
}
  
ResultSetPtr PipeBase::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, p);
  
  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  runPvPython
  (
    cm, executionPath(), list_of<std::string>
    (
      init+
      "eb = planarSlice(cbi, [0,0,0], [0,0,1])\n"
      "Show(eb)\n"
      "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
      "setCam([0,0,10], [0,0,0], [0,1,0])\n"
      "WriteImage('pressure_longi.jpg')\n"
    )
  );
  results->insert("pressureContour",
    std::auto_ptr<Image>(new Image
    (
    "pressure_longi.jpg", 
    "Contour of pressure (longitudinal section)", ""
  )));
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	init+
	"eb = planarSlice(cbi, [0,0,0], [0,0,1])\n"
	"Show(eb)\n"
	"displayContour(eb, 'U', arrayType='CELL_DATA', component="+lexical_cast<char>(i)+", barpos=[0.5,0.7], barorient=0)\n"
	"setCam([0,0,10], [0,0,0], [0,1,0])\n"
	"WriteImage('U"+c+"_longi.jpg')\n"
      )
    );
    results->insert("U"+c+"Contour",
      std::auto_ptr<Image>(new Image
      (
      "U"+c+"_longi.jpg", 
      "Contour of "+c+"-Velocity", ""
    )));
  }

  
  return results;
}

defineType(PipeCyclic);

PipeCyclic::PipeCyclic(const NoParameters& nop)
: PipeBase(nop)
{
}

void PipeCyclic::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  PipeBase::createMesh(cm, p);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void PipeCyclic::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));

  PipeBase::createCase(cm, p);
}

addToFactoryTable(Analysis, PipeCyclic, NoParameters);


}
