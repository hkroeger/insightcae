#include "latextable.h"
#include "latexdocumentation.h"

#include "base/boost_include.h"
#include "base/exception.h"

#include "base/parameter.h"
#include "base/parameters/subsetparameter.h"
#include "base/parameters/selectablesubsetparameter.h"


using namespace insight;

LatexTable::LatexTable(
    const std::string& label,
    const std::string& description,
    size_t nCols,
    const std::vector<double>& widthFractions,
    const std::vector<std::string>& colHeads
    )
  : nCols_(nCols),
    description_(description),
    colHeads_(colHeads),
    label_(label)
{
  if (widthFractions.size()>nCols_)
    throw std::runtime_error("too many width fractions");
  if (widthFractions.size()<nCols_-1)
    throw std::runtime_error("too few width fractions");

  double total=0.;
  for (const auto w: widthFractions)
  {
    total+=w;
    widthFractions_.push_back(w);
  }
  if (widthFractions_.size()==nCols_-1)
    widthFractions_.push_back(1.-total);
}

void LatexTable::append(const std::vector<std::string> &line)
{
  if (line.size()!=nCols_)
    throw insight::Exception("attempt to add a line with wrong number of columns!");
  push_back(line);
}


void LatexTable::print(std::ostream& os) const
{
  os << "\\begin{table}[h]\n";
  os << "\\begin{tabular}{";
  for (const auto& w: widthFractions_)
    os << "p{"<<w<<"\\linewidth}";
  os<<"}\n";
  os << "\\hline\n";
  os << boost::algorithm::join(colHeads_, " & ") << "\\\\\n";
  os << "\\hline\n";
  for (const auto& l: *this)
  {
    os << boost::algorithm::join(l, " & ") << "\\\\\n";
  }
  os << "\\hline\n";
  os << "\\end{tabular}\n";
  os << "\\caption{"+description_+"}\n";
  os << "\\label{"<<label_<<"}\n";
  os << "\\end{table}\n";
}


void generateLatexTable(
    LatexDocumentation& doc,
    const std::string& label,
    const std::string& description,
    const insight::ParameterSet& ps,
    const std::string& prefix)
{
  auto tabPtr = std::make_shared<LatexTable>(label, description, 2);
  auto& tab = *tabPtr;
  doc.push_back(tabPtr);


  for (auto i=ps.begin(); i!=ps.end(); ++i)
  {
    const auto& label = i->first;

    auto ppath=prefix+"/"+label;

    auto ppath_clean = ppath;
    boost::replace_all(ppath_clean, "/", "_");

    const auto* pptr = i->second.get();

    auto lxlabel=SimpleLatex(label).toLaTeX();
    auto lxdesc=pptr->description().toLaTeX();

    if (const auto* subp = dynamic_cast<const SubsetParameter*>(pptr))
    {
      generateLatexTable(doc, ppath_clean, "Parameters of sub dict "+ppath, subp->subset(), ppath);

      lxdesc+="\n\nSee table \\ref{"+ppath_clean+"} for the description of this sub dict.\n";
    }
    else if (const auto* ssubp = dynamic_cast<const SelectableSubsetParameter*>(pptr))
    {

      std::vector<std::string> refs;

      for (const auto& item: ssubp->items())
      {
        auto sellabel = item.first;

        if (item.second->size()>0)
        {
          generateLatexTable(
                doc,
                ppath_clean+"_"+sellabel,
                "Parameters of sub dict "+ppath+" for selection "+SimpleLatex(sellabel).toLaTeX(),
                *item.second,
                ppath);

          refs.push_back( "table \\ref{"+ppath_clean+"_"+sellabel+"} for "+SimpleLatex(sellabel).toLaTeX() );
        }
      }

      if (refs.size()>0)
      {
        lxdesc+=
            "\n\nSee for the description of the sub parameters for the different selections: "
            + boost::algorithm::join(refs, ", ");
      }

    }
    else if (const auto* subarr = dynamic_cast<const ArrayParameter*>(pptr))
    {

    }

    tab.append({ lxlabel, lxdesc });
  }

}
