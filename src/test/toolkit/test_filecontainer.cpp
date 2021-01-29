
#include "base/exception.h"
#include "base/tools.h"
#include "base/filecontainer.h"

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
  std::string content="Hallo", content2="Du da";

  boost::filesystem::path fp, fp2;

  {
    FileContainer fc( boost::filesystem::path("x")/"y"/"test.txt", std::make_shared<std::string>(content+"\n") );

    fp=fc.filePath();
    checkFileContent(fp, content);

    sleep(2); // acces time resolution is 1 second

    fc.replaceContentBuffer( std::make_shared<std::string>(content2) );
    fp2=fc.filePath();
    insight::assertion(fp==fp2, "file name has changed! Now:"+fp2.string()+", previously: "+fp.string());
    checkFileContent(fp2, content2);
  }

  GlobalTemporaryDirectory::clear();

  insight::assertion( !boost::filesystem::exists(fp), "temporary file was not cleared");
  insight::assertion( !boost::filesystem::exists(fp2), "temporary file 2 was not cleared");


  return 0;
}
