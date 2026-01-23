#include "relativemovingbody.h"

namespace Foam {


relativeMovingBody::relativeMovingBody(const dictionary &dict, const polyMesh& mesh)
 : IndependentMovingMeshPart(dict, mesh),
   smbf_(solidBodyMotionFunction::New(dict, mesh.time()))
{}


septernion relativeMovingBody::transformation() const
{
    return smbf_->transformation();
}


autoPtr<relativeMovingBody> relativeMovingBody::clone() const
{
    return autoPtr<relativeMovingBody>(new relativeMovingBody(*this));
}


} // namespace Foam
