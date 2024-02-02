#ifndef STITCHEDCOMPOUND_H
#define STITCHEDCOMPOUND_H

#include "cadfeature.h"

namespace insight {
namespace cad {


class StitchedCompound
: public Feature
{
  FeatureSetPtr faces_;
  ScalarPtr tol_;

  virtual size_t calcHash() const;
  virtual void build();

  StitchedCompound(FeatureSetPtr faces, ScalarPtr tol=scalarconst(1e-3) );

public:
  declareType("StitchedCompound");
  CREATE_FUNCTION(StitchedCompound);

  static FeatureCmdInfoList ruleDocumentation();
  static void insertrule(parser::ISCADParser& ruleset);
};

}
}

#endif // STITCHEDCOMPOUND_H
