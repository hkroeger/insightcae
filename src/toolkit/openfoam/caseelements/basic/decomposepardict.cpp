#include "decomposepardict.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(decomposeParDict);
addToOpenFOAMCaseElementFactoryTable(decomposeParDict);

decomposeParDict::decomposeParDict(OpenFOAMCase& c, ParameterSetInput ip)
  : OpenFOAMCaseElement(c, /*"decomposeParDict", */ip.forward<Parameters>())
{}



std::vector<int> factors(int n)
{
  // do a prime factorization of n

  std::vector<int> facs;
  int z = 2;

  while (z * z <= n)
  {
      if (n % z == 0)
      {
          facs.push_back(z);
          n /= z;
      }
      else
      {
          z++;
      }
  }

  if (n > 1)
  {
      facs.push_back( n );
  }

  return facs;
}

std::vector<int> combinefactors
(
  std::vector<int> facs,
  const std::tuple<int,int,int>& po
)
{
  // count locked directions (without decomposition)
  int n_lock=0;
  if (std::get<0>(po)<=0.0) n_lock++;
  if (std::get<1>(po)<=0.0) n_lock++;
  if (std::get<2>(po)<=0.0) n_lock++;
  // if less than 3 factors or directions are locked: extend with ones
  int n_add=std::max(3-int(facs.size()), n_lock);
  for (int k=0;k<n_add;k++)
    facs.push_back(1);

  // bring factors into descending order
  sort(facs.begin(), facs.end());
  std::reverse(facs.begin(), facs.end());

  // get initial number which was factored
  double totprod=1.0;
  for (int f: facs) totprod*=double(f);

  double potot=std::get<0>(po)+std::get<1>(po)+std::get<2>(po);
  std::vector<double> pof =
  {
    (double(std::get<0>(po))/potot),
    (double(std::get<1>(po))/potot),
    (double(std::get<2>(po))/potot)
  };

  std::vector<std::size_t> pof_sorti(pof.size());
  std::iota(pof_sorti.begin(), pof_sorti.end(), 0);
  std::sort(pof_sorti.begin(), pof_sorti.end(), [&pof](std::size_t left, std::size_t right)
  {
      return pof[left] > pof[right];
  });

  std::vector<int> nf(3);
  size_t j=0;

  for (size_t i=0; i<3; i++)
  {
    size_t dir_idx=pof_sorti[i];
    double req_frac=pof[dir_idx];
    int cf=facs[j++];
    while (j<facs.size()-(2-i) && (cf>=0.0) && ( (log(cf)/log(totprod)) < req_frac) )
    {
      {
        cf*=facs[j++];
      }
    }
    nf[dir_idx]=cf;
  }

  return nf;
}


void decomposeParDict::addIntoDictionaries ( OFdicts& dictionaries ) const
{
  auto pom=p().decompWeights;

  std::tuple<int,int,int> po(int(pom(0)), int(pom(1)), int(pom(2)));

  OFDictData::dict& decomposeParDict=dictionaries.lookupDict("system/decomposeParDict");

// #warning hack for testing
  std::vector<int> ns=combinefactors(factors(p().np), po);
  std::cout<<"decomp "<<p().np<<": "<<ns[0]<<" "<<ns[1]<<" "<<ns[2]<<std::endl;
  decomposeParDict["numberOfSubdomains"]=p().np;

  if (p().decompositionMethod==Parameters::hierarchical)
  {
    decomposeParDict["method"]="hierarchical";
  }
  else if (p().decompositionMethod==Parameters::simple)
  {
    decomposeParDict["method"]="simple";
  }
  else if (p().decompositionMethod==Parameters::scotch)
  {
    decomposeParDict["method"]="scotch";
  }
  else if (p().decompositionMethod==Parameters::metis)
  {
    decomposeParDict["method"]="metis";
  }
  else
  {
      throw insight::Exception("setDecomposeParDict: internal error (unhandled decomposition method)!");
  }

  {
    OFDictData::dict coeffs;
    coeffs["n"]=OFDictData::vector3(ns[0], ns[1], ns[2]);
    coeffs["delta"]=0.001;
    coeffs["order"]="xyz";
    decomposeParDict["hierarchicalCoeffs"]=coeffs;
  }

  {
    OFDictData::dict coeffs;
    coeffs["n"]=OFDictData::vector3(ns[0], ns[1], ns[2]);
    coeffs["delta"]=0.001;
    decomposeParDict["simpleCoeffs"]=coeffs;
  }
}


std::string decomposeParDict::category()
{
  return "Numerics";
}


} // namespace insight
