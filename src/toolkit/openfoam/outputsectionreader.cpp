#include "outputsectionreader.h"

namespace insight {


defineType(OutputSectionReader);


defineStaticFunctionTable2(
    "output section parsers",
    OutputSectionReader, Readers, createIfMatches );

bool OutputSectionReader::parseNextLine(const std::string &line)
{
    return false;
}


} // namespace insight
