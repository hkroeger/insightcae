#ifndef INSIGHT_STREAMREDIRECTOR_H
#define INSIGHT_STREAMREDIRECTOR_H

#include <iostream>
#include <mutex>

namespace insight {

class StreamRedirector
     : public std::basic_streambuf<char>

{
    std::ostream &streamToRedirect_;
    std::streambuf *oldBuffer_;

    std::string currentLine_;

    void processCurrentLine();

public:
    StreamRedirector(std::ostream &streamToRedirect);
    ~StreamRedirector();

protected:
    //This is called when a std::endl has been inserted into the stream
    int_type overflow(int_type v) override;
    std::streamsize xsputn(const char *p, std::streamsize n) override;

    virtual void processLine(std::string line) =0;
};

} // namespace insight

#endif // INSIGHT_STREAMREDIRECTOR_H
