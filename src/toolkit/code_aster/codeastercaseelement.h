#ifndef CODEASTERCASEELEMENT_H
#define CODEASTERCASEELEMENT_H

#include "base/caseelement.h"

#include "code_aster/codeastercase.h"

namespace insight {

class CodeAsterCaseElement
: public CaseElement
{
public:
    CodeAsterCaseElement(CodeAsterCase& c, const std::string& name, const ParameterSet& ps);
};

} // namespace insight

#endif // CODEASTERCASEELEMENT_H
