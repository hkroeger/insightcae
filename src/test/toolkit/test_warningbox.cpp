
#include "base/exception.h"
#include "base/boost_include.h"

using namespace boost;

int main(int argc, char* argv[])
{

  std::vector<std::string> warningTexts = {

    "It is a long established fact that a reader will be distracted by the readable content of a page when looking at its layout."
    " The point of using Lorem Ipsum is that it has a more-or-less normal distribution of letters, as opposed to using 'Content here, content here', making it look like readable English."
    " Many desktop publishing packages and web page editors now use Lorem Ipsum as their default model text, and a search for 'lorem ipsum' will uncover many web sites still in their infancy."
    " Various versions have evolved over the years, sometimes by accident, sometimes on purpose (injected humour and the like).",

    "Prism layer covering is only "+str(format("%g")%(2.32843e-308))+"\% (<90%)!\n"
    "Please reconsider prism layer thickness and tune number of prism layers!",

    "Prism layer covering is only "+str(format("%g")%(89.422))+"\% (<90%)!\n"
    "Please reconsider prism layer thickness and tune number of prism layers!",

    "decomposeParDict does not contain proper number of processors!\n"
   +str(format("(%d != %d)\n") % 1 % 89 )
   +"It will be recreated but the directional preferences cannot be taken into account.\n"
    "Correct this by setting the np parameter in FVNumerics during case creation properly.",

    "This is a warning with a very long file path:\n"
    "/bla/blu/blaa/ffddd/fgdjgdlkfgjdgkjdgkdfgjl/gdflgjdlfkgjdflgkjd/wqoirwofinKÖBAKRGUBALBALBFLDABFGUDGB.dwg\n"
    "sss",

    "This is a warning with a very long file path:\n"
    "/bla/blu/blaa/ffddd/fgdjgdlkfgjdgkjdgkdfgjl/gdflgjdlfkgjdflgkjd/wqoirwofinKÖBAKRGUBALBALBFLDABFGUDGB.dwg"

  };

  for (const auto& wt: warningTexts)
    insight::Warning(wt);
  return 0;
}
