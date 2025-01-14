
#include "base/exception.h"
#include "base/boost_include.h"

#include "base/tools.h"
#include "base/filecontainer.h"
#include "base/zipfile.h"

using namespace insight;

int main(int argc, char* argv[])
{
    try
    {
        boost::filesystem::path filePath("testzip.zip");
        std::string testzipBase64="UEsDBAoAAAAAAE9ZeVTZiBFhBwAAAAcAAAAFABwAZmlsZTFVVAkAAxaVPWI6lT1idXgLAAEE6AMAAAToAwAASGFsbG8xClBLAwQKAAAAAADRs3lUAAAAAAAAAAAAAAAABQAcAGRpcjEvVVQJAAN6ND5igTQ+YnV4CwABBOgDAAAE6AMAAFBLAwQKAAAAAADRs3lUcgtJShIAAAASAAAACgAcAGRpcjEvZmlsZTFVVAkAA3o0PmJ6ND5idXgLAAEE6AMAAAToAwAASGFsbG8yCkR1MgpkYTIKISEKUEsDBAoAAAAAAFtZeVQAAAAAAAAAAAAAAAAKABwAZGlyMS9kaXIyL1VUCQADLZU9YmuVPWJ1eAsAAQToAwAABOgDAABQSwMECgAAAAAAW1l5VFvqJ1MHAAAABwAAAA8AHABkaXIxL2RpcjIvZmlsZTFVVAkAAy2VPWJrlT1idXgLAAEE6AMAAAToAwAASGFsbG8zClBLAQIeAwoAAAAAAE9ZeVTZiBFhBwAAAAcAAAAFABgAAAAAAAEAAAC0gQAAAABmaWxlMVVUBQADFpU9YnV4CwABBOgDAAAE6AMAAFBLAQIeAwoAAAAAANGzeVQAAAAAAAAAAAAAAAAFABgAAAAAAAAAEAD9QUYAAABkaXIxL1VUBQADejQ+YnV4CwABBOgDAAAE6AMAAFBLAQIeAwoAAAAAANGzeVRyC0lKEgAAABIAAAAKABgAAAAAAAEAAAC0gYUAAABkaXIxL2ZpbGUxVVQFAAN6ND5idXgLAAEE6AMAAAToAwAAUEsBAh4DCgAAAAAAW1l5VAAAAAAAAAAAAAAAAAoAGAAAAAAAAAAQAP1B2wAAAGRpcjEvZGlyMi9VVAUAAy2VPWJ1eAsAAQToAwAABOgDAABQSwECHgMKAAAAAABbWXlUW+onUwcAAAAHAAAADwAYAAAAAAABAAAAtIEfAQAAZGlyMS9kaXIyL2ZpbGUxVVQFAAMtlT1idXgLAAEE6AMAAAToAwAAUEsFBgAAAAAFAAUAiwEAAG8BAAAAAA==";

        writeStringIntoFile(base64_decode(testzipBase64), filePath);

        ZipFile zf(filePath);

        auto fl = zf.files();
        for (const auto& e: fl)
        {
            std::cout<<e<<std::endl;
        }

        auto fls=zf.uncompressFiles();
        for (const auto& e: fls)
        {
            std::cout<<e.first<<": "<<(*e.second)<<std::endl;
        }

        auto fls2=zf.uncompressFiles({"file1"});
        for (const auto& e: fls2)
        {
            std::cout<<e.first<<": "<<(*e.second)<<std::endl;
        }

        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
