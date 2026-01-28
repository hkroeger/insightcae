#ifndef UNIOF_HELPERS_H
#define UNIOF_HELPERS_H


#include "uniof_time.h"
#include "Pstream.H"
#include "OFstream.H"

namespace Foam
{




template<const char* fName>
class FileOnDemand
{
    word name_;
    const Time& time_;
    mutable autoPtr<OFstream> filePtr_;

public:
    FileOnDemand(const word& name, const Time& time)
        : name_(name), time_(time)
    {}

    virtual ~FileOnDemand()
    {
        filePtr_.reset();
    }

    virtual void writeFileHeader(OFstream& os)
    {}

    Ostream& operator()()
    {
        // Create the minmax file if not already created
        if (filePtr_.empty())
        {
            // File update
            if (Pstream::master())
            {
                fileName dir;
                word startTimeName =
                    time_.timeName(time_.startTime().value());

                if (Pstream::parRun())
                {
                    // Put in undecomposed case (Note: gives problems for
                    // distributed data running)
                    dir = time_.path()/".."/"postProcessing"/name_/startTimeName;
                }
                else
                {
                    dir = time_.path()/"postProcessing"/name_/startTimeName;
                }

                // Create directory if does not exist.
                mkDir(dir);

                // Open new file at start up
                filePtr_.reset(new OFstream(dir/fName));

                // Add headers to output data
                writeFileHeader(filePtr_());
            }
            else
            {
                filePtr_.reset(new OFstream("/dev/null"));
            }
        }

        return filePtr_();
    }
};


}

#endif // UNIOF_HELPERS_H
