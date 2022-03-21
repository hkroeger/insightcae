#ifndef INSIGHT_OFENVIRONMENT_H
#define INSIGHT_OFENVIRONMENT_H

#include "base/softwareenvironment.h"

#include "base/boost_include.h"

namespace insight {




class OFEnvironment
    : public SoftwareEnvironment
{
protected:
    int version_;
    boost::filesystem::path bashrc_;

public:
    OFEnvironment ( int version, const boost::filesystem::path& bashrc );

    virtual int version() const;
    virtual const boost::filesystem::path& bashrc() const;
};




} // namespace insight

#endif // INSIGHT_OFENVIRONMENT_H
