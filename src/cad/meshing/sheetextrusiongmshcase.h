#ifndef INSIGHT_CAD_SHEETEXTRUSIONGMSHCASE_H
#define INSIGHT_CAD_SHEETEXTRUSIONGMSHCASE_H


#include "meshing/gmshcase.h"

namespace insight {
namespace cad {


class SheetExtrusionGmshCase
    : public GmshCase
{

public:
  typedef std::pair<std::string, cad::FeatureSetPtr> NamedEntity;

protected:
  std::map<cad::FeatureID, std::string>
      namedBottomFaces_,
      namedTopFaces_,
      namedLateralEdges_;

  double grading_;

public:
  SheetExtrusionGmshCase(
      cad::ConstFeaturePtr part,
      const std::string& solidName,
      const boost::filesystem::path& outputMeshFile,
      double L, double h, int nLayers,
      const std::vector<NamedEntity>& namedBottomFaces,
      const std::vector<NamedEntity>& namedTopFaces,
      const std::vector<NamedEntity>& namedLateralEdges,
      double grading=1.,
      bool keepDir=false,
      bool recombineTris = true
      );
};


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SHEETEXTRUSIONGMSHCASE_H
