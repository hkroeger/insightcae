#ifndef INSIGHT_EXTERNALPROGRAMS_H
#define INSIGHT_EXTERNALPROGRAMS_H

#include "base/boost_include.h"

namespace insight {

class ExternalPrograms
        : public std::map<std::string, boost::filesystem::path>
{
    ExternalPrograms();

public:
    ExternalPrograms(const ExternalPrograms& o);

    boost::filesystem::path firstWritableLocation() const;
    void writeConfiguration(const boost::filesystem::path& file);

    std::vector<std::string> missingPrograms() const;

    static ExternalPrograms& globalInstance();
    static boost::filesystem::path path(const std::string& exeName);
};

} // namespace insight

#endif // INSIGHT_EXTERNALPROGRAMS_H
