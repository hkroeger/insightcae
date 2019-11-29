#ifndef INSIGHT_OFES_H
#define INSIGHT_OFES_H

#include "openfoam/ofenvironment.h"

#include <vector>
#include <string>

namespace insight {


class OFEs
    : public boost::ptr_map<std::string, OFEnvironment>
{
public:
    static OFEs list;

    static std::vector<std::string> all();
    static const OFEnvironment& get ( const std::string& name );

    /**
     * inspects WM_PROJECT_DIR env variable and returns name of currently loaded OFE. Empty string, if none is set
     */
    static std::string detectCurrentOFE();
    static std::string currentOrPreferredOFE();
    static const OFEnvironment& getCurrent ( );
    static const OFEnvironment& getCurrentOrPreferred();

    OFEs();
    ~OFEs();
};


} // namespace insight

#endif // INSIGHT_OFES_H
