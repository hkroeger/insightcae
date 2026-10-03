#include "base/exception.h"

#include "cadfeature.h"
#include "cadmodel.h"
#include "parser.h"


using namespace insight;
using namespace insight::cad;


struct ErrorCase
{
    std::string script;
    int line, column;
    std::vector<std::string> messageContains;
};


int main(int, char*[])
{
    std::vector<ErrorCase> cases = {

        // missing closing parenthesis inside a command
        { "D1=10;\n"
          "c1: Cylinder(O, ax EX, D1 ;\n",
          2, 27, { "in Cylinder(...)", "expected ')' but found ';'", "Usage: Cylinder" } },

        // undefined symbol with a similar name
        { "D1=10;\n"
          "c1: Cylinder(O, ax EX, Dx);\n",
          2, 24, { "undefined symbol 'Dx'", "Did you mean 'D1'" } },

        // symbol of wrong type
        { "p=[1,0,0];\n"
          "D=1;\n"
          "c1: Cylinder(O, ax EX, p);\n",
          3, 24, { "defined as point" } },

        // command name used as symbol
        { "Cylinder: Cylinder(O, ax EX, 1);\n",
          1, 1, { "'Cylinder' cannot be used as a symbol name (feature command of the same name exists)" } },

        // missing semicolon in feature assignment:
        // reported at the end of the line
        { "c1: Cylinder(O, ax EX, 1)\n"
          "c2: Cylinder(O, ax EY, 1);\n",
          1, 26, { "expected ';'" } },

        // missing semicolon in "=" assignment
        { "a=1\n"
          "b=2;\n",
          1, 3, { "Maybe the ';' at the end of line 1 is missing?" } },

        // unknown keyword
        { "a=1;\n"
          "@descriptio\n",
          2, 1, { "unknown keyword '@descriptio'", "Did you mean '@description'?" } },

        // unknown postprocessing command
        { "c1: Cylinder(O, ax EX, 1);\n"
          "SolidPropertes(p) << c1;\n",
          2, 1, { "unknown postprocessing command 'SolidPropertes'", "Did you mean 'SolidProperties'?" } },

        // unknown command
        { "c1: Cylindr(O, ax EX, 1);\n",
          1, 5, { "undefined symbol 'Cylindr'", "Did you mean 'Cylinder'?" } },

        // missing assignment operator
        { "a 1;\n",
          1, 1, { "expected ':', '=', '?=' or '->' after 'a'" } },

        // property assignment to non-feature
        { "a=1;\n"
          "a->density=1;\n",
          2, 1, { "properties can only be assigned to features" } },

        // unknown property
        { "c1: Cylinder(O, ax EX, 1);\n"
          "c1->dens=1;\n",
          2, 5, { "'density'" } },

        // '%' in the source excerpt must not be interpreted as format directive
        { "a = 1 %s;\n",
          1, 5, { "a = 1 %s;" } },

        // error in nested command
        { "c1: Cylinder(O, ax EX, volume(Box(O, EX, EY EZ)));\n",
          1, 45, { "in Box(...)", "expected ','" } }
    };

    int failed=0;
    for (const auto& c: cases)
    {
        auto m = std::make_shared<cad::Model>();
        try
        {
            parseISCADModel(c.script, m.get());
            std::cerr << "FAIL: no error raised for script:\n" << c.script << std::endl;
            ++failed;
        }
        catch (const insight::cad::parser::iscadParserException& e)
        {
            std::cout << e.message() << "\n" << std::endl;

            bool ok = (e.line()==c.line && e.column()==c.column);
            if (!ok)
            {
                std::cerr << "FAIL: expected location " << c.line << ":" << c.column
                          << ", got " << e.line() << ":" << e.column() << std::endl;
            }
            for (const auto& s: c.messageContains)
            {
                if (e.message().find(s)==std::string::npos)
                {
                    std::cerr << "FAIL: message does not contain \"" << s << "\"" << std::endl;
                    ok=false;
                }
            }
            if (!ok) ++failed;
        }
    }

    // valid scripts must still parse
    std::vector<std::string> validScripts = {

        // postprocessing statements are allowed everywhere
        "D1=10;\n"
        "c1: Cylinder(O, ax EX, D1);\n"
        "SolidProperties(p1) << c1;\n"
        "c1->density=7.8;\n"
        "c2: Cylinder(O, ax EY, D1);\n"
        "SolidProperties(p2) << c2;\n",

        // deprecated "@post" keyword is ignored (backwards compatibility)
        "D1=10;\n"
        "c1: Cylinder(O, ax EX, D1);\n"
        "c1->density=7.8;\n"
        "@post\n",

        "D1=10;\n"
        "c1: Cylinder(O, ax EX, D1);\n"
        "@post\n"
        "SolidProperties(p1) << c1;\n"
        "c2: Cylinder(O, ax EY, D1);\n"
    };

    for (const auto& scr: validScripts)
    {
        auto m = std::make_shared<cad::Model>();
        try
        {
            if (!parseISCADModel(scr, m.get()))
            {
                std::cerr << "FAIL: valid script not parsed:\n" << scr << std::endl;
                ++failed;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "FAIL: valid script raised " << e.what() << "\n" << scr << std::endl;
            ++failed;
        }
    }

    std::cout << failed << " of " << cases.size()+validScripts.size() << " cases failed." << std::endl;
    return failed ? -1 : 0;
}
