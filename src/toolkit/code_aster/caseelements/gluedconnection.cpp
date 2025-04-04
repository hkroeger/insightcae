#include "gluedconnection.h"

namespace insight {

GluedConnection::GluedConnection(CodeAsterCase& c, ParameterSetInput ip)
    : CodeAsterCaseElement(c, ip.forward<Parameters>())
{}

} // namespace insight
