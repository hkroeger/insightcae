#ifndef INSIGHT_OFES_H
#define INSIGHT_OFES_H

#include "openfoam/ofenvironment.h"

#include <vector>
#include <string>

#include "boost/ptr_container/ptr_map.hpp"

namespace insight {


class OFEs
    : public std::map<std::string, std::unique_ptr<OFEnvironment> >
{
    OFEs();
    ~OFEs();

public:
    static OFEs& list();

    static std::vector<std::string> all();
    static const OFEnvironment& get ( const std::string& name );

    /**
     * inspects WM_PROJECT_DIR env variable and returns name of currently loaded OFE. Empty string, if none is set
     */
    static std::string detectCurrentOFE(bool *currentOFEDefined=nullptr);
    static std::string currentOrPreferredOFE();
    static std::string preferredOFE();
    static const OFEnvironment& getCurrent ( );
    static const OFEnvironment& getPreferred();
    static const OFEnvironment& getCurrentOrPreferred();

};


} // namespace insight

#endif // INSIGHT_OFES_H
