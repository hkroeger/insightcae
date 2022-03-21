#include "latexdocumentation.h"




using namespace insight;




LatexDocumentation::LatexDocumentation(const ParameterSet& ps,
                                       const std::string& labelprefix)
{
    generateLatexTable(
          *this,
          "global_parameters", "Parameter set",
          ps, "", 20,
          labelprefix );
}




void LatexDocumentation::print(std::ostream& os, const std::string& labelPrefix) const
{
  for (const auto& tab: *this)
  {
    tab->print(os, labelPrefix);
    os<<"\n\n";
  }
}
