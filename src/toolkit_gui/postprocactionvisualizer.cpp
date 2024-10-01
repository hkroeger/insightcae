#include "postprocactionvisualizer.h"

namespace insight {
namespace cad {


defineType(PostProcActionVisualizers);


defineStaticFunctionTableWithArgs(
    PostProcActionVisualizers,
    createAISReprByTypeName,
    Handle_AIS_InteractiveObject,
    LIST(insight::cad::PostprocActionPtr ppa),
    LIST(ppa)
    );



Handle_AIS_InteractiveObject
PostProcActionVisualizers::createAISReprByTypeName(PostprocActionPtr)
{
  return Handle_AIS_InteractiveObject(); // nothing to visualize by default
}



Handle_AIS_InteractiveObject
PostProcActionVisualizers::createAISRepr(PostprocActionPtr ppa)
{
  try
  {
    ppa->checkForBuildDuringAccess();
    return createAISReprByTypeNameFor(ppa->type(), ppa);
  }
  catch (insight::Exception& e)
  {
    insight::Warning(e);
  }

  return PostProcActionVisualizers::createAISReprByTypeName(ppa);
}



PostProcActionVisualizers postProcActionVisualizers;

} // namespace cad
} // namespace insight
