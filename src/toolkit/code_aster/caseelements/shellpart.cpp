#include "shellpart.h"

namespace insight {

ShellPart::ShellPart(CodeAsterCase& c, ParameterSetInput ip)
    : CodeAsterCaseElement(c, ip.forward<Parameters>())
{}

} // namespace insight
