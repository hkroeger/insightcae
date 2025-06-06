#ifndef GLUEDCONNECTION_H
#define GLUEDCONNECTION_H

#include "code_aster/codeastercaseelement.h"

namespace insight {

class GluedConnection
:public CodeAsterCaseElement
{
public:
    GluedConnection(CodeAsterCase& c, ParameterSetInput ip = Parameters() );
};

} // namespace insight

#endif // GLUEDCONNECTION_H
