#include "sheetextrusiongmshcase.h"

#include "cadfeature.h"

namespace insight {
namespace cad {

using namespace std;
using namespace boost;

SheetExtrusionGmshCase::SheetExtrusionGmshCase(
    cad::ConstFeaturePtr part,
    const std::string& solidName,
    const boost::filesystem::path& outputMeshFile,
    double L, double h, int nLayers,
    const std::vector<NamedEntity>& namedBottomFaces,
    const std::vector<NamedEntity>& namedTopFaces,
    const std::vector<NamedEntity>& namedLateralEdges,
    double grading,
    bool keepDir,
    bool recombineTris
    )
  : cad::GmshCase(part, outputMeshFile,
                  L, L, keepDir),
    grading_(grading)
{
  insight::assertion(grading_>0., "grading must be larger than zero!");

  for (const auto& nbf: namedBottomFaces)
  {
    for (const auto& fi: nbf.second->data())
    {
      namedBottomFaces_[fi]=nbf.first;
    }
  }
  for (const auto& ntf: namedTopFaces)
  {
    for (const auto& fi: ntf.second->data())
    {
      namedTopFaces_[fi]=ntf.first;
    }
  }
  for (const auto& nle: namedLateralEdges)
  {
    for (const auto& ei: nle.second->data())
    {
      namedLateralEdges_[ei]=nle.first;
    }
  }

  if (recombineTris)
  {
    insertLinesBefore(endOfMeshingOptions_, {
      "Mesh.RecombinationAlgorithm = 0",
      "Mesh.RecombineAll = 1",
     });
  }

  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.SecondOrderIncomplete=1",
    "Mesh.Optimize = 1",
    "Physical Volume(\""+solidName+"\") = {}"
   });

  auto insertPhysical = [&](const std::string& entityTypeName, const std::map<cad::FeatureID, std::string>& nfs)
  {
    std::set<std::string> names;

    std::transform(nfs.begin(), nfs.end(),
                   std::inserter(names, names.begin()),
                   [](const std::map<cad::FeatureID, std::string>::value_type& i) { return i.second; } );

    for (const auto& nbf: names)
    {
      insertLinesBefore(endOfMeshingOptions_, {
                          "Physical "+entityTypeName+"(\""+nbf+"\")={}"
                        });
    }
  };

  insertPhysical("Surface", namedBottomFaces_);
  insertPhysical("Surface", namedTopFaces_);
  insertPhysical("Surface", namedLateralEdges_);


  // insert faces one by one
  auto faces=part->allFacesSet();

  std::string layerSpecification;

  if (fabs(1.-grading_)<1e-10)
  {
    layerSpecification =
      str( format("Layers{%d}") % nLayers );
  }
  else
  {
    double g=pow(grading_, 1./double(nLayers-1));

    arma::mat h = arma::ones(nLayers);
    for (int i=1; i<nLayers; ++i)
      h(i)=g*h(i-1);
    h/=arma::as_scalar(sum(h));

    arma::mat hcum = arma::zeros(nLayers);
    hcum(0)=h(0);
    for (int i=1; i<nLayers; ++i)
    {
      hcum(i)=hcum(i-1)+h(i);
    }

    std::string layerNum,layerHeight;
    for (int i=0;i<nLayers;++i)
    {
      layerNum += "1";
      layerHeight += boost::lexical_cast<std::string>( hcum(i) );
      if (i<nLayers-1)
      {
        layerNum+=",";
        layerHeight+=",";
      }
    }
    layerSpecification="Layers{ {"+layerNum+"}, {"+layerHeight+"} }";
  }

  for (FeatureID fi : faces)
  {
    std::string out=str(format("out%d")%fi);

    insertLinesBefore(endOfMeshingActions_, {
      str(format(out+"[] = Extrude {0.,0.,%g} { Surface{%d}; %s; Recombine; }")
                        % h % fi % layerSpecification ),
      "Physical Volume(\""+solidName+"\") += "+out+"[1]"
    });
  }


  for (FeatureID fi : faces)
  {
    std::string out=str(format("out%d")%fi);

    // get list of the edge IDs, sort by their ID
    std::vector<FeatureID> currentFaceEdges;
    for (TopExp_Explorer ex(part->face(fi), TopAbs_EDGE); ex.More(); ex.Next())
    {
      currentFaceEdges.push_back(part->edgeID(ex.Current()));
    }
    std::sort(currentFaceEdges.begin(), currentFaceEdges.end());

    auto nbf=namedBottomFaces_.find(fi);
    if (nbf!=namedBottomFaces_.end())
    {
      insertLinesBefore(endOfMeshingActions_, {
        str(format("Physical Surface(\"%s\") += {%d}") % nbf->second % nbf->first)
                        });
    }
    auto ntf=namedTopFaces_.find(fi);
    if (ntf!=namedTopFaces_.end())
    {
      insertLinesBefore(endOfMeshingActions_, {
        str(format("Physical Surface(\"%s\") += %s[0]") % ntf->second % out)
                        });
    }

    for (size_t i=0; i<currentFaceEdges.size(); i++)
    {
      auto eid = currentFaceEdges[i];
      auto nle = namedLateralEdges_.find(eid);
      if (nle!=namedLateralEdges_.end())
      {
        insertLinesBefore(endOfMeshingActions_, {
          str(format("Physical Surface(\"%s\") += %s[%d]")
                            % nle->second % out % (2+i) )
                          });
      }
    }
  }

}


} // namespace cad
} // namespace insight
