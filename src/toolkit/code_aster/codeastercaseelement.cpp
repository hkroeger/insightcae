#include "codeastercaseelement.h"

namespace insight {

CodeAsterCaseElement::CodeAsterCaseElement(
    CodeAsterCase& c, ParameterSetInput ip
    )
    : CaseElement(c, ip.forward<Parameters>())
{}

} // namespace insight
