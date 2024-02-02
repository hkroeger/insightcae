#include "streamredirector.h"

#include "base/exception.h"
#include <vector>
#include "boost/algorithm/string.hpp"


namespace insight {


std::mutex mutex_;

void StreamRedirector::processCurrentLine()
{
    processLine(currentLine_);
    currentLine_="";
}

StreamRedirector::StreamRedirector(std::ostream &streamToRedirect)
  : streamToRedirect_( streamToRedirect )
{
    oldBuffer_ = streamToRedirect.rdbuf ( this );
}

StreamRedirector::~StreamRedirector()
{
    streamToRedirect_.rdbuf ( oldBuffer_ );
}


//This is called when a std::endl has been inserted into the stream
StreamRedirector::int_type StreamRedirector::overflow ( StreamRedirector::int_type v )
{
  mutex_.lock();
  if ( v == '\n' )
    {
      processCurrentLine();
    }
  mutex_.unlock();
  return v;
}


std::streamsize StreamRedirector::xsputn ( const char *p, std::streamsize n )
{
  mutex_.lock();
  std::string str ( p, n );

  if ( boost::find_first ( str, "\n" ) )
    {
      std::vector<std::string> strSplitted;
      boost::split ( strSplitted, str, boost::is_any_of ( "\n" ) );

      currentLine_ += strSplitted[0];
      processCurrentLine();

      for ( int i = 1; i < strSplitted.size()-1; i++ )
        {
          currentLine_ = strSplitted[i];
          processCurrentLine();
        }

      if ( strSplitted.size() >1 )
        {
          currentLine_ += strSplitted[strSplitted.size()-1];
        }
    }
  else
    {
      currentLine_ += str;
    }
  mutex_.unlock();
  return n;
}


} // namespace insight
