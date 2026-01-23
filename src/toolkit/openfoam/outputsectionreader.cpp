#include "outputsectionreader.h"

namespace insight {


defineType(OutputSectionReader);


defineStaticFunctionTable2(
    "output section parsers",
    OutputSectionReader, Readers, createIfMatches );


} // namespace insight
