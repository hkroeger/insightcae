
#include "base/exception.h"
#include "base/tools.h"
#include "base/filecontainer.h"
#include "base/parameters/pathparameter.h"
#include "base/rapidxml.h"

#include "boost/thread.hpp"

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
    FileContainer fc( std::make_shared<std::string>(content1+"\n"), boost::filesystem::path("x")/"y"/"test.txt" );
    PathParameter pp( fc, "test file" );

    fp=pp.filePath();
    timespec cmt1=pp.contentModificationTime();
    cout<<"check 1: "<<fp<<" "<<cmt1<<endl;
    checkFileContent(fp, content1);

    boost::this_thread::sleep_for( boost::chrono::seconds(1) );
//    sleep(1);

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


  {
    // regression test: setFilePath must resolve a relative path against
    // baseDirectory_, not against the process's current working directory
    boost::filesystem::path base = boost::filesystem::unique_path(
        boost::filesystem::temp_directory_path()/"fc_setfilepath_base_%%%%%%");
    boost::filesystem::create_directories(base/"models");

    FileContainer fc2(boost::filesystem::path(), base);

    auto savedCwd = boost::filesystem::current_path();
    boost::filesystem::current_path(boost::filesystem::temp_directory_path());

    fc2.setFilePath( boost::filesystem::path("models")/"part.stl" );

    boost::filesystem::current_path(savedCwd);

    insight::assertion(
        fc2.expandedFilePath(true) == base/"models"/"part.stl",
        "setFilePath resolved relative path against the wrong base directory: got "
            + fc2.expandedFilePath(true).string() );

    boost::filesystem::remove_all(base);
  }

  {
    // regression test: appendToNode must serialize a forward-relative path
    // (relative to baseDirectory_ reaching filePath_), not a backwards one
    boost::filesystem::path base = boost::filesystem::unique_path(
        boost::filesystem::temp_directory_path()/"fc_appendtonode_base_%%%%%%");
    boost::filesystem::create_directories(base/"sub");
    boost::filesystem::path absFile = base/"sub"/"file.stl";
    {
      std::ofstream f(absFile.string());
      f<<"x";
    }

    FileContainer fc3(absFile, base);

    rapidxml::xml_document<> doc;
    rapidxml::xml_node<>* node = doc.allocate_node(rapidxml::node_element, "test");
    doc.append_node(node);
    fc3.appendToNode(doc, *node);

    auto *attr = node->first_attribute("value");
    insight::assertion(bool(attr), "fileName attribute missing");
    insight::assertion(
        boost::filesystem::path(attr->value())
            == boost::filesystem::path("sub")/"file.stl",
        "appendToNode produced a backwards-relative path, got: "
            + std::string(attr->value()) );

    boost::filesystem::remove_all(base);
  }

  return 0;
}
