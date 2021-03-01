
#include "base/exception.h"
#include "base/tools.h"
#include "base/filecontainer.h"
#include "base/parameters/pathparameter.h"

using namespace insight;

void checkFileContent(const boost::filesystem::path& p, const std::string& content)
{
  std::ifstream f(p.string());
  std::string line;
  getline(f, line);
  std::cout<<"First line in "<<p<<": >>>"<<line<<"<<<"<<std::endl;
  insight::assertion(line==content, "unexpected content!");
}

int main(int /*argc*/, char*/*argv*/[])
{
  std::string content1="Hallo", content2="Du da";

  boost::filesystem::path fp, fp2;

  {
    FileContainer fc( boost::filesystem::path("x")/"y"/"test.txt", std::make_shared<std::string>(content1+"\n") );
    PathParameter pp( fc, "test file" );

    fp=pp.filePath();
    timespec cmt1=pp.contentModificationTime();
    cout<<"check 1: "<<fp<<" "<<cmt1<<endl;
    checkFileContent(fp, content1);

    sleep(1);

    // access a second time
    fp=pp.filePath();
    timespec cmt2=pp.contentModificationTime();

    insight::assertion(cmt1==cmt2, "modification times should not have changed");

    cout<<"check 1b: "<<fp<<" "<<cmt2<<endl;
    checkFileContent(fp, content1);

//    sleep(2); // acces time resolution is 1 second

    pp.replaceContentBuffer( std::make_shared<std::string>(content2) );
    fp2=pp.filePath();
    insight::assertion(fp==fp2, "file name has changed! Now:"+fp2.string()+", previously: "+fp.string());
    cout<<"check 2:"<<fp<<" "<<pp.contentModificationTime()<<endl;
    checkFileContent(fp2, content2);
  }

  GlobalTemporaryDirectory::clear();

  insight::assertion( !boost::filesystem::exists(fp), "temporary file was not cleared");
  insight::assertion( !boost::filesystem::exists(fp2), "temporary file 2 was not cleared");


  return 0;
}
