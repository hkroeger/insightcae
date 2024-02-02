#ifndef INSIGHT_CAD_SINGLEVERTEX_H
#define INSIGHT_CAD_SINGLEVERTEX_H

#include "cadfeature.h"

namespace insight {
namespace cad {

class SingleVertex
: public Feature
{
  VectorPtr p_; // location

  size_t calcHash() const override;
  void build() override;

  SingleVertex(VectorPtr p);

public:
  declareType("SingleVertex");
  CREATE_FUNCTION(SingleVertex);

  static void insertrule(parser::ISCADParser& ruleset);
  static FeatureCmdInfoList ruleDocumentation();
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SINGLEVERTEX_H
