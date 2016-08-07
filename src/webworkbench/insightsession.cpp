
#include "insightsession.h"

namespace insight
{
namespace web
{
    
Session::Session()
{
    dir_ = boost::filesystem::temp_directory_path();
    boost::filesystem::create_directories(dir_);
}


Session::~Session()
{
    remove_all(dir_);
}
    
}
}
