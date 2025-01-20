#ifndef SHELLPART_H
#define SHELLPART_H

#include "code_aster/codeastercaseelement.h"

#include "shellpart__ShellPart__Parameters_headers.h"

namespace insight {

class ShellPart
    : public CodeAsterCaseElement
{
public:
#include "shellpart__ShellPart__Parameters.h"

/*
PARAMETERSET>>> ShellPart Parameters
inherits CodeAsterCaseElement::Parameters

geometry = path "" "Geometry of the shell part"
thickness = double 1 "Shell thickness"

createGetters
<<<PARAMETERSET
*/

public:
    ShellPart(CodeAsterCase& c, ParameterSetInput ip = Parameters() );
};

} // namespace insight

#endif // SHELLPART_H
