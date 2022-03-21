#include "casedirectory.h"




#include "base/tools.h"
#include "base/exception.h"




namespace insight {




CaseDirectory::CaseDirectory(const boost::filesystem::path& p)
  : boost::filesystem::path( boost::filesystem::canonical(p) ),
    keep_(true),
    isAutoCreated_(false)
{
  insight::assertion( !p.empty(), "internal error: the case directory path must not be empty!");
  insight::assertion( boost::filesystem::exists(p), "internal error: the case directory "+p.string()+" has to exist!");
}




CaseDirectory::CaseDirectory(bool keep, const boost::filesystem::path& prefix)
: keep_(keep),
  isAutoCreated_(true)
{
  if (getenv("INSIGHT_KEEPTEMPDIRS"))
    keep_=true;

  path fn=prefix.filename();

  auto parentcasedir=absolute(prefix.parent_path());

  if (!directoryIsWritable(parentcasedir))
  {
    insight::dbg() << "directory is NOT writable: "<<parentcasedir<<std::endl;
    parentcasedir=boost::filesystem::temp_directory_path();
    insight::dbg() << "diverting to "<<parentcasedir<<std::endl;
  }
  else
  {
    insight::dbg() << "directory is writable: "<<parentcasedir<<std::endl;
  }

  boost::filesystem::path::operator=(
              unique_path(
                parentcasedir
                /
                (timeCodePrefix() + "_" + fn.string() + "_%%%%")
               )
        );

  createDirectory();
}




CaseDirectory::~CaseDirectory()
{
  if (!keep_)
  {
    insight::CurrentExceptionContext ex("removing case directory "+this->string());
    try
    {
      remove_all(*this);
    }
    catch (const std::exception& e)
    {
      insight::Warning(e.what());
    }
  }
}




bool CaseDirectory::isAutoCreated() const
{
  return isAutoCreated_;
}




bool CaseDirectory::isExistingAndWillBeRemoved() const
{
  return !keep_
          && boost::filesystem::exists(*this)
          && boost::filesystem::is_directory(*this);
}




bool CaseDirectory::isExistingAndNotEmpty() const
{
  if (
          boost::filesystem::exists(*this)
          && boost::filesystem::is_directory(*this)
          )
  {
    int n=0;
    for (boost::filesystem::directory_iterator di(*this);
         di!=boost::filesystem::directory_iterator(); ++di)
      ++n;
    return n>0;
  }
  return false;
}




void CaseDirectory::createDirectory()
{
  if (!boost::filesystem::exists(*this))
  {
    insight::CurrentExceptionContext ex("creating case directory "+this->string());
    create_directories(*this);
  }
}




bool CaseDirectory::isPersistent() const
{
    return keep_;
}




void CaseDirectory::makePersistent()
{
    keep_=true;
}

CaseDirectoryPtr CaseDirectory::makeTemporary(const boost::filesystem::path& prefix)
{
    return std::make_shared<CaseDirectory>(false, prefix);
}




} // namespace insight
