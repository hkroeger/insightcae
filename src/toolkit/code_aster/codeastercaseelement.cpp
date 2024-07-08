#include "codeastercaseelement.h"

namespace insight {

CodeAsterCaseElement::CodeAsterCaseElement(
    CodeAsterCase& c, const std::string& name, const ParameterSet& ps
    )
    : CaseElement(c, name, ps)
{}

} // namespace insight
