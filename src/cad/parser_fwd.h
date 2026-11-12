#ifndef INSIGHT_CAD_PARSER_FWD_H
#define INSIGHT_CAD_PARSER_FWD_H

#include <memory>

/*
 * Forward declarations of the ISCAD parser types.
 *
 * Include this instead of parser.h in headers, which only
 * need to mention the parser in declarations. parser.h pulls in
 * boost::spirit qi/karma/phoenix, which costs >1 GB of compiler
 * memory per translation unit. Only translation units, which
 * actually define grammar rules (insertrule, addParserRule etc.),
 * should include parser.h.
 */

namespace insight {
namespace cad {
namespace parser {

struct ISCADParser;
class SyntaxElementDirectory;
typedef std::shared_ptr<SyntaxElementDirectory> SyntaxElementDirectoryPtr;

} // namespace parser
} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_PARSER_FWD_H
