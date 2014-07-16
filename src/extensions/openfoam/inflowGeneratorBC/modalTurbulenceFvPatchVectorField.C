 

#include "modalTurbulenceFvPatchVectorField.H" 

#include "transform.H"
#include "transformField.H"
#include "volFields.H"
#include "ListListOps.H"
#include "PstreamReduceOps.H"
#include "addToRunTimeSelectionTable.H"
#include "globalMeshData.H"
#include "globalIndex.H"
#include "wallDist.H"

#include "base/vtktools.h"

#include <vector>
#include "boost/lexical_cast.hpp"
#include "boost/foreach.hpp"

using namespace std;
using namespace boost;

namespace Foam 
{


//defineTypeNameAndDebug(modalTurbulenceFvPatchVectorField, 0);

void modalTurbulenceFvPatchVectorField::writeStateVisualization
(
  int i,
  const vectorField& u,
  const vectorField* uMean,
  const symmTensorField* uPrime2Mean
) const
{
//   insight::vtk::vtkModel vtk_vortons;
//   
//   vtk_vortons.setPoints(
//     vortons_.size(),
//     vortons_.component(vector::X)().data(),
//     vortons_.component(vector::Y)().data(),
//     vortons_.component(vector::Z)().data()
//     );
//   
//   for (label k=0; k<3; k++)
//   {
//     std::vector<double> Lx, Ly, Lz;
//     forAll(vortons_, j)
//     {
//       const vector& L = vortons_[j].L(k);
//       Lx.push_back(L.x()); Ly.push_back(L.y()); Lz.push_back(L.z());
//     }
//     vtk_vortons.appendPointVectorField("L"+lexical_cast<string>(k), Lx.data(), Ly.data(), Lz.data());
//   }
//   
//   insight::vtk::vtkModel2d vtk_patch;
//   // set cells
//   const polyPatch& ppatch = patch().patch();
//   vtk_patch.setPoints
//   (
//     ppatch.localPoints().size(), 
//     ppatch.localPoints().component(vector::X)().data(),
//     ppatch.localPoints().component(vector::Y)().data(),
//     ppatch.localPoints().component(vector::Z)().data()
//   );
//   for(label fi=0; fi<ppatch.size(); fi++)
//   {
//     const face& f = ppatch.localFaces()[fi];
//     vtk_patch.appendPolygon(f.size(), f.cdata());
//   }
//   
//   vtk_patch.appendCellVectorField
//   (
//     "u", 
//     u.component(vector::X)().cdata(),
//     u.component(vector::Y)().cdata(),
//     u.component(vector::Z)().cdata()
//   );
//   if (uMean)
//   {
//     vtk_patch.appendCellVectorField
//     (
//       "uMean", 
//       uMean->component(vector::X)().cdata(),
//       uMean->component(vector::Y)().cdata(),
//       uMean->component(vector::Z)().cdata()
//     );
//   }
//   if (uPrime2Mean)
//   {
//     vtk_patch.appendCellTensorField
//     (
//       "R", 
//       uPrime2Mean->component(symmTensor::XX)().cdata(),
//       uPrime2Mean->component(symmTensor::XY)().cdata(),
//       uPrime2Mean->component(symmTensor::XZ)().cdata(),
//       uPrime2Mean->component(symmTensor::XY)().cdata(),
//       uPrime2Mean->component(symmTensor::YY)().cdata(),
//       uPrime2Mean->component(symmTensor::YZ)().cdata(),
//       uPrime2Mean->component(symmTensor::XZ)().cdata(),
//       uPrime2Mean->component(symmTensor::YZ)().cdata(),
//       uPrime2Mean->component(symmTensor::ZZ)().cdata()
//     );
//   }
//   {
//     
//     IOobject oo
//     (
//       "vortons_"+this->patch().name()+"_"+lexical_cast<string>(i)+".vtk",
//       this->db().time().timeName(),
//       this->db().time(),
//       IOobject::NO_READ,
//       IOobject::AUTO_WRITE
//     );
//     IOobject oop
//     (
//       "patch_"+this->patch().name()+"_"+lexical_cast<string>(i)+".vtk",
//       this->db().time().timeName(),
//       this->db().time(),
//       IOobject::NO_READ,
//       IOobject::AUTO_WRITE
//     );
//     mkDir(oo.path());
//     
//     Info<<"Writing "<<oo.objectPath()<<endl;
//     std::ofstream f(oo.objectPath().c_str());
//     vtk_vortons.writeLegacyFile(f);
//     f.close();
//     
//     Info<<"Writing "<<oop.objectPath()<<endl;
//     std::ofstream f2(oop.objectPath().c_str());
//     vtk_patch.writeLegacyFile(f2);
//     f2.close();
//   }
}


bool modalTurbulenceFvPatchVectorField::mode::operator!=(const mode& other) const
{
  return other.k!=k;
}

modalTurbulenceFvPatchVectorField::modalTurbulenceFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    inflowGeneratorBaseFvPatchVectorField(p, iF),
    modes_(),
    tau_(-1)
{
}

modalTurbulenceFvPatchVectorField::modalTurbulenceFvPatchVectorField
(
    const modalTurbulenceFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    inflowGeneratorBaseFvPatchVectorField(ptf, p, iF, mapper),
    modes_(),
    tau_(-1)
{
}

modalTurbulenceFvPatchVectorField::modalTurbulenceFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    inflowGeneratorBaseFvPatchVectorField(p, iF, dict),
    modes_(),
    tau_(-1)
{
    if (dict.found("modes"))
    {
      tau_=readScalar(dict.lookup("tau"));
      modes_=List<mode>(dict.lookup("modes"));
    }  
}

modalTurbulenceFvPatchVectorField::modalTurbulenceFvPatchVectorField
(
    const modalTurbulenceFvPatchVectorField& ptf
)
: inflowGeneratorBaseFvPatchVectorField(ptf),
  modes_(ptf.modes_),
  tau_(ptf.tau_)
{}

modalTurbulenceFvPatchVectorField::modalTurbulenceFvPatchVectorField
(
    const modalTurbulenceFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
: inflowGeneratorBaseFvPatchVectorField(ptf, iF),
  modes_(ptf.modes_),
  tau_(ptf.tau_)
{}


void modalTurbulenceFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
  inflowGeneratorBaseFvPatchVectorField::autoMap(m);
  forAll(modes_, i)
  {
    modes_[i].q.autoMap(m);
  }
}


void modalTurbulenceFvPatchVectorField::rmap
(
    const fvPatchField<vector>& ptf,
    const labelList& addr
)
{
  inflowGeneratorBaseFvPatchVectorField::rmap(ptf, addr);
  
  const modalTurbulenceFvPatchVectorField& tiptf = 
    refCast<const modalTurbulenceFvPatchVectorField >(ptf);
  forAll(modes_, i)
  {
    modes_[i].q.rmap(tiptf.modes_[i].q, addr);
  }
}

void modalTurbulenceFvPatchVectorField::createModes()
{
  const polyPatch& ppatch = patch().patch();

  wallDist ywall(patch().boundaryMesh().mesh());
  
  const scalarField& dw=ywall.boundaryField()[patch().index()];
  scalarField delta_x= 2.*(patch().Cf() - ppatch.faceCellCentres()) & patch().nf();
  scalarField delta_max_edge(size(), 0.0);
  forAll(ppatch.edges(), ei)
  {
    const edge& e = ppatch.edges()[ei];
    scalar this_edge_len=e.mag(ppatch.localPoints());
    forAll(ppatch.edgeFaces()[ei], j)
    {
      label fi = ppatch.edgeFaces()[ei][j];
      delta_max_edge[fi] = max(delta_max_edge[fi], this_edge_len);
    }
  }
  scalarField delta_max=max(delta_max_edge, delta_x);
  scalarField lcut=2.*min
  (
    max(delta_max_edge, 0.3*delta_max) + 0.1*dw,
    delta_max
  );
  
  scalar ce=3., ctau=2.;
  scalar alpha=0.025;
#warning viscosity is hard-coded!
  scalar nu=1e-5;
  
  scalarField lt=mag(L());
  scalarField le=min(2.*dw, ce*lt);
  scalarField ke=2.*M_PI/le;
  
  scalar lemax=gMax(le);
  scalar kmin = 2.*M_PI/lemax;
  scalarField kcut = 2.*M_PI/lcut;
  scalar kcutmax=gMax(kcut);
  scalarField keta=2.*M_PI/ (pow(nu,3)*lt/pow(mag(Umean())+SMALL, 3));
  tau_=ctau*lemax/ ( gSum(-Umean()&patch().Sf()) / gSum(patch().magSf()) );
  
  {
    std::vector<scalar> ks;
    scalar kn=0;
    
    const label nmax=10000;
    while ( (kn=kmin*::pow(1.+alpha, ks.size())) < 1.5*kcutmax )
    {
      ks.push_back(kn);
      if (ks.size()>=nmax)
      {
	FatalErrorIn("modalTurbulenceFvPatchVectorField")
	<< "maximum number of modes reached ("<<nmax<<"): kn="<<kn  <<", limit kn="<< 1.5*kcutmax
	<<abort(FatalError);
      }
    }
    
    modes_.resize(ks.size());
    
    // var_nor: Generator for random numbers with normal distribution
    boost::mt19937 rng;
    boost::normal_distribution<> nd(2.*M_PI, 2.*M_PI);
    boost::variate_generator<boost::mt19937&, boost::normal_distribution<> > var_nor(rng, nd);
    
    scalarField qsum(size(), 0.0);
    
    for (size_t i=0; i<ks.size(); i++)
    {
      mode& m = modes_[i];
      m.k=ks[i];
      const scalar& k=m.k;
      
      // compute random numbers on master processor and distribute afterwards
      // => need to be consistent over all processors
      if (Pstream::master())
      {
	m.d = 2.*vector(ranGen_()-0.5, ranGen_()-0.5, ranGen_()-0.5);
	m.d/=SMALL+mag(m.d);
	
	m.phi=ranGen_()*2.*M_PI;
	
	m.s=var_nor();
	
	m.sigma=vector(ranGen_(), ranGen_(), 0);
	m.sigma.z() = - m.sigma&m.d / m.d.z();
	m.sigma /= SMALL+mag(m.sigma);
      }
      else 
      {
	m.d=vector::zero;
	m.phi=0.0;
	m.s=0.0;
	m.sigma=vector::zero;
      }
      
      reduce(m.d, sumOp<vector>());
      reduce(m.phi, sumOp<scalar>());
      reduce(m.s, sumOp<scalar>());
      reduce(m.sigma, sumOp<vector>());
            
      scalarField feta=exp(-pow(12.*k/keta,2));
      scalarField fcut=exp(-pow(4*max(k-0.9*kcut,0.0)/kcut, 3));
      scalarField Ek= feta*fcut*
	pow(k/ke, 4) 
	/ 
	pow(1.+2.4*pow(k/ke, 2), 17./6.);
	
      scalar delta_k=k;
      if (i>0) delta_k-=ks[i-1];
      
      m.q=Ek*delta_k;
      qsum+=m.q;
    }
    forAll(modes_, i)
    {
      modes_[i].q/=qsum;
    }
    Info<<"Created n="<<modes_.size()<<" modes."<<endl;
  }
}


tmp<vectorField> modalTurbulenceFvPatchVectorField::continueFluctuationProcess(scalar t, ProcessStepInfo *info)
{

  if (modes_.size()==0) createModes();
  
  tmp<vectorField> tfluctuations(new vectorField(size(), vector::zero));
  vectorField& fluctuations = tfluctuations();
  
  
  forAll(modes_, mi)
  {
    const mode& m = modes_[mi];
    fluctuations += sqrt(m.q) * (m.sigma * cos( ((m.k*m.d)&patch().Cf()) + m.phi + m.s*t/tau_));
  }
  fluctuations *= 2.*sqrt(3./2.);

  if (info) 
  {
//     info->n_removed=n_removed;
//     info->n_generated=n_generated;
//     info->n_total=n_total;
  }

  if (debug>=3)
  {
   Pout<<" fluctuations: min/max/avg = "<<min(fluctuations)<<" / "<<max(fluctuations) << " / "<<average(fluctuations)<<endl;
   forAll(fluctuations, j)
    if (mag(fluctuations[j])>1e3) Pout<<j<<": "<<tfluctuations<<endl;
  }

  return tfluctuations;
}


void modalTurbulenceFvPatchVectorField::write(Ostream& os) const
{
  if (modes_.size()>0)
  {
    os.writeKeyword("tau") << tau_ << token::END_STATEMENT <<nl;
    os.writeKeyword("modes") << modes_ << token::END_STATEMENT <<nl;
  }
    
  inflowGeneratorBaseFvPatchVectorField::write(os);
}

Ostream& operator<<(Ostream& os, const modalTurbulenceFvPatchVectorField::mode& m)
{
  os << m.k <<nl;
  os << m.phi <<nl;
  os << m.s <<nl;
  os << m.d <<nl;
  os << m.sigma <<nl;
  os << m.q <<nl;
  return os;
}

Istream& operator>>(Istream& os, modalTurbulenceFvPatchVectorField::mode& m)
{
  m.k=readScalar(os);
  m.phi=readScalar(os);
  m.s=readScalar(os);
  m.d=vector(os); 
  m.sigma=vector(os);
  m.q=scalarField(os);
  return os;
}

makePatchTypeField
(
    fvPatchVectorField,
    modalTurbulenceFvPatchVectorField
);

}
