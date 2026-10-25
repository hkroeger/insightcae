#include "base/exception.h"

#include "cadfeature.h"
#include "cadmodel.h"
#include "parser.h"
#include "cadfeatures/parametricsketch.h"

#include <regex>


using namespace insight;
using namespace insight::cad;
using namespace insight::cad::parser;


int nFailed=0;

void check(bool cond, const std::string& msg)
{
    if (!cond)
    {
        std::cerr<<"FAILED: "<<msg<<std::endl;
        ++nFailed;
    }
}


/**
 * byte position of the n-th (0-based) occurrence of word as a whole identifier
 */
long find(const std::string& script, const std::string& word, int n=0)
{
    std::regex re("\\b"+word+"\\b");
    int i=0;
    for (auto it=std::sregex_iterator(script.begin(), script.end(), re);
         it!=std::sregex_iterator(); ++it, ++i)
    {
        if (i==n) return it->position();
    }
    throw insight::Exception("word %s (occurrence %d) not found in script", word.c_str(), n);
}


std::string text(const std::string& script, const SyntaxElementPos& r)
{
    return script.substr(r.first, r.second-r.first);
}


void checkToken(
    const SyntaxElementDirectory& sd,
    const std::string& script,
    const std::string& word, int n,
    TokenKind expectedKind )
{
    auto p=find(script, word, n);
    std::string label="token \""+word+"\" #"+std::to_string(n)+" at "+std::to_string(p);
    auto it=sd.tokens.find(p);
    check(it!=sd.tokens.end(), label+" was not recorded");
    if (it!=sd.tokens.end())
    {
        check(text(script, it->second.range)==word,
              label+" has wrong range: \""+text(script, it->second.range)+"\"");
        check(it->second.kind==expectedKind,
              label+" has wrong kind "+std::to_string(int(it->second.kind)));
    }
}


const FoldRange* foldRangeAt(const SyntaxElementDirectory& sd, long p)
{
    auto it=sd.foldRanges.find(p);
    if (it==sd.foldRanges.end()) return nullptr;
    return &it->second;
}


void testValidScript()
{
    std::string scr=
        "L=15;\n"
        "v=L*EX;\n"
        "pl=Plane(O, EZ);\n"
        "a: Cylinder(O, v, 2);\n"
        "vol=volume(a);\n"
        "f=a?faces('isPlane');\n"
        "w  =  2 ;\n"
        "c : Cylinder ( O, v, w )  # trailing comment\n"
        ";\n"
        "psk=ParametricSketch( pl,\n"
        " SketchPoint(1, 0, 0),\n"
        " SketchPoint(2, 3, 0.1),\n"
        " Line(3, 1, 2)\n"
        ");\n"
        "# a comment with non-ASCII characters: \xc3\xa4\xc3\xb6\xc3\xbc\n"
        "m: for(i,1,2,{\n"
        " s: Sphere(i*EX, L/10);\n"
        " q: ParametricSketch(XY, SketchPoint(1, 0, 0));\n"
        " inner: for(j,1,2,{ t: Sphere(j*EY, 0.1); });\n"
        "});\n"
        ;

    auto m = std::make_shared<cad::Model>();
    SyntaxElementDirectoryPtr sd;
    int failloc=-1;
    if (!parseISCADModel(scr, m.get(), &failloc, &sd))
    {
        throw insight::Exception("Parser failed at \"%s\"", scr.substr(failloc).c_str());
    }

    checkToken(*sd, scr, "L", 0, TokenKind::ScalarDef);
    checkToken(*sd, scr, "v", 0, TokenKind::VectorDef);
    checkToken(*sd, scr, "L", 1, TokenKind::ScalarRef);
    checkToken(*sd, scr, "EX", 0, TokenKind::VectorRef);
    checkToken(*sd, scr, "pl", 0, TokenKind::DatumDef);
    checkToken(*sd, scr, "a", 0, TokenKind::ComponentDef);
    checkToken(*sd, scr, "Cylinder", 0, TokenKind::FeatureCommand);
    checkToken(*sd, scr, "v", 1, TokenKind::VectorRef);
    checkToken(*sd, scr, "vol", 0, TokenKind::ScalarDef);
    checkToken(*sd, scr, "volume", 0, TokenKind::ScalarFunction);
    checkToken(*sd, scr, "a", 1, TokenKind::FeatureRef);
    checkToken(*sd, scr, "f", 0, TokenKind::SelectionDef);
    checkToken(*sd, scr, "psk", 0, TokenKind::FeatureDef);
    checkToken(*sd, scr, "ParametricSketch", 0, TokenKind::FeatureCommand);
    checkToken(*sd, scr, "pl", 1, TokenKind::DatumRef);

    // whitespace and comments must not be part of the ranges
    checkToken(*sd, scr, "w", 0, TokenKind::ScalarDef);
    checkToken(*sd, scr, "c", 0, TokenKind::ComponentDef);
    checkToken(*sd, scr, "Cylinder", 1, TokenKind::FeatureCommand);
    checkToken(*sd, scr, "w", 1, TokenKind::ScalarRef);

    // inside in-situ submodels (positions must be absolute)
    checkToken(*sd, scr, "m", 0, TokenKind::ComponentDef);
    checkToken(*sd, scr, "for", 0, TokenKind::FeatureCommand);
    checkToken(*sd, scr, "s", 0, TokenKind::ComponentDef);
    checkToken(*sd, scr, "Sphere", 0, TokenKind::FeatureCommand);
    checkToken(*sd, scr, "i", 1, TokenKind::ScalarRef);
    checkToken(*sd, scr, "L", 2, TokenKind::ScalarRef);
    checkToken(*sd, scr, "ParametricSketch", 1, TokenKind::FeatureCommand);
    checkToken(*sd, scr, "XY", 0, TokenKind::DatumRef);
    checkToken(*sd, scr, "inner", 0, TokenKind::ComponentDef);
    checkToken(*sd, scr, "t", 0, TokenKind::ComponentDef);
    checkToken(*sd, scr, "j", 1, TokenKind::ScalarRef);

    // command names
    check(sd->commands.count("Cylinder")
            && sd->commands.at("Cylinder")==CommandKind::FeatureCommand,
          "command Cylinder not registered");
    check(sd->commands.count("volume")
            && sd->commands.at("volume")==CommandKind::ScalarFunction,
          "function volume not registered");

    // fold ranges
    auto checkCommandFold = [&](const std::string& cmd, int n, const std::string& expectedEnd)
    {
        auto p=find(scr, cmd, n);
        auto *fr=foldRangeAt(*sd, p);
        check(fr!=nullptr, "no fold range for "+cmd+" #"+std::to_string(n));
        if (fr)
        {
            auto t=text(scr, fr->range);
            check(fr->command==cmd, "fold range has wrong command name "+fr->command);
            check(t.size()>=expectedEnd.size()
                    && t.substr(t.size()-expectedEnd.size())==expectedEnd,
                  "fold range of "+cmd+" #"+std::to_string(n)+" has wrong end: \""+t+"\"");
        }
    };
    checkCommandFold("ParametricSketch", 0, "Line(3, 1, 2)\n)");
    checkCommandFold("Cylinder", 0, "(O, v, 2)");
    checkCommandFold("Cylinder", 1, "( O, v, w )");
    checkCommandFold("for", 0, "});\n})");
    checkCommandFold("for", 1, "0.1); })");

    {
        auto p=scr.find('{');
        auto *fr=foldRangeAt(*sd, p);
        check(fr!=nullptr, "no fold range for outer submodel");
        if (fr)
        {
            check(fr->command.empty(), "submodel fold range has a command name");
            check(text(scr, fr->range)==scr.substr(p, scr.rfind('}')-p+1),
                  "outer submodel range is wrong: \""+text(scr, fr->range)+"\"");
        }
        auto p2=scr.find('{', p+1);
        auto *fr2=foldRangeAt(*sd, p2);
        check(fr2!=nullptr, "no fold range for inner submodel");
        if (fr2)
        {
            check(text(scr, fr2->range)=="{ t: Sphere(j*EY, 0.1); }",
                  "inner submodel range is wrong: \""+text(scr, fr2->range)+"\"");
        }
    }

    // script ranges of parametric sketches (also inside submodels)
    int nSketches=0;
    for (const auto& e: *sd)
    {
        if (auto *fp=boost::get<FeaturePtr>(&e.second))
        {
            if (auto ps=std::dynamic_pointer_cast<ParametricSketch>(*fp))
            {
                auto r=ps->scriptRange();
                auto t=scr.substr(r.first, r.second-r.first);
                if (e.first.second.first==find(scr, "ParametricSketch", 0))
                {
                    check(t.find("SketchPoint(1, 0, 0),")==0
                          && t.find("Line(3, 1, 2)")!=std::string::npos,
                          "wrong script range of top level sketch: \""+t+"\"");
                    ++nSketches;
                }
                else if (e.first.second.first==find(scr, "ParametricSketch", 1))
                {
                    check(t.find("SketchPoint(1, 0, 0)")==0,
                          "wrong script range of sketch in submodel: \""+t+"\"");
                    ++nSketches;
                }
            }
        }
    }
    check(nSketches==2, "expected 2 parametric sketches, found "+std::to_string(nSketches));
}




void testIncompleteScript()
{
    std::string scr=
        "L=15;\n"
        "b: Cylinder(O, L*EX, 2);\n"
        "c: Cylinder(O, , 2);\n"
        "d: Sphere(O, 1);\n"
        ;

    auto m = std::make_shared<cad::Model>();
    SyntaxElementDirectoryPtr sd;
    int failloc=-1;
    bool failed=false;
    try
    {
        parseISCADModel(scr, m.get(), &failloc, &sd);
    }
    catch (const iscadParserException&)
    {
        failed=true;
    }
    check(failed, "parsing of the incomplete script did not fail");
    check(bool(sd), "no syntax element directory after failure");
    if (sd)
    {
        checkToken(*sd, scr, "L", 0, TokenKind::ScalarDef);
        checkToken(*sd, scr, "b", 0, TokenKind::ComponentDef);
        checkToken(*sd, scr, "Cylinder", 0, TokenKind::FeatureCommand);
        checkToken(*sd, scr, "L", 1, TokenKind::ScalarRef);
    }
}




int main(int, char*argv[])
{
    try
    {
        testValidScript();
        testIncompleteScript();
    }
    catch (insight::Exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    if (nFailed)
    {
        std::cerr<<nFailed<<" checks failed."<<std::endl;
        return -1;
    }
    return 0;
}
