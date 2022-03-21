#ifndef INSIGHT_CAD_POSTPROCACTIONVISUALIZER_H
#define INSIGHT_CAD_POSTPROCACTIONVISUALIZER_H

#include "cadtypes.h"
#include "cadpostprocaction.h"

#include "base/factory.h"

namespace insight {
namespace cad {

class PostProcActionVisualizers
{
public:
  declareStaticFunctionTableWithArgs(
      createAISReprByTypeName,
      Handle_AIS_InteractiveObject,
      LIST(insight::cad::PostprocActionPtr ppa),
      LIST(insight::cad::PostprocActionPtr ppa)
      );
  declareType("PostProcActionVisualizers");

public:
  Handle_AIS_InteractiveObject createAISRepr( insight::cad::PostprocActionPtr ppa );

  static Handle_AIS_InteractiveObject createAISReprByTypeName(insight::cad::PostprocActionPtr ppa);
};

extern PostProcActionVisualizers postProcActionVisualizers;


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_POSTPROCACTIONVISUALIZER_H
