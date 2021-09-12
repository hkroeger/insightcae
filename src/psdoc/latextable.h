#ifndef LATEXTABLE_H
#define LATEXTABLE_H

#include <string>
#include <vector>
#include <memory>

#include "base/parameterset.h"

class LatexDocumentation;

class LatexTable
  : protected std::vector<std::vector<std::string> >
{
  size_t nCols_;
  std::string description_;
  std::vector<double> widthFractions_;
  std::vector<std::string> colHeads_;
  std::string label_;

public:
  LatexTable(
      const std::string& label,
      const std::string& description,
      size_t nCols=2,
      const std::vector<double>& widthFractions={0.25, 0.75},
      const std::vector<std::string>& colHeads={"Parameter", "Description"}
      );

  void append(const std::vector<std::string>& line);

  void print(std::ostream& os) const;
};

typedef std::shared_ptr<LatexTable> LatexTablePtr;

void generateLatexTable(
    LatexDocumentation& doc,
    const std::string& label,
    const std::string& description,
    const insight::ParameterSet& ps,
    const std::string& prefix = "");

#endif // LATEXTABLE_H
