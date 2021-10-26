#include "latexdocumentation.h"




using namespace insight;




LatexDocumentation::LatexDocumentation(const ParameterSet& ps)
{
    generateLatexTable(*this, "global_parameters", "Parameter set", ps);
}




void LatexDocumentation::print(std::ostream& os) const
{
  for (const auto& tab: *this)
  {
    tab->print(os);
    os<<"\n\n";
  }
}
