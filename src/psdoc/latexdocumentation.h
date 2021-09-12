#ifndef LATEXDOCUMENTATION_H
#define LATEXDOCUMENTATION_H

#include <map>
#include <ostream>

#include "latextable.h"

#include "base/parameterset.h"

class LatexDocumentation
    : public std::vector<LatexTablePtr>
{
public:
  LatexDocumentation(const insight::ParameterSet& ps);

  void print(std::ostream& os) const;
};

#endif // LATEXDOCUMENTATION_H
