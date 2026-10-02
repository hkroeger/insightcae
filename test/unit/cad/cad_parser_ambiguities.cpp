#include "base/exception.h"

#include "cadfeature.h"
#include "cadmodel.h"
#include "parser.h"

#include <cmath>
#include <functional>


using namespace insight;
using namespace insight::cad;


// Each case is a script which must parse completely (or must be
// rejected, if mustFail is set), optionally followed by a check
// of the resulting model.
struct ParserCase
{
    std::string name;
    std::string script;
    std::function<bool(const Model&)> check;
    bool mustFail = false;
};


bool isClose(double a, double b)
{
    return std::fabs(a-b) < 1e-10;
}


int main(int, char*[])
{
    const std::string box = "B: Box(O, EX, EY, EZ);";

    std::vector<ParserCase> cases = {

        // feature names which start with a feature command name
        { "feature name with command prefix",
          "Box1: Box(O, EX, EY, EZ); V=volume(Box1);", nullptr },

        // feature names which start with a vector function name
        { "feature name with vector function prefix",
          "rotor: Box(O, EX, EY, EZ); F=rotor?faces('isPlane');", nullptr },

        // point names which start with a scalar function name
        { "point name with scalar function prefix",
          "position=[1,2,3]; s=position.x;",
          [](const Model& m) { return isClose(m.lookupScalar("s")->value(), 1.); } },

        // keyword "in" must not match the start of "inplane"
        { "RefPlace inplane condition",
          box+"R: RefPlace(B, O inplane XY);", nullptr },

        // vector division must bind like multiplication
        { "vector division precedence",
          "v=[4,0,0]/2*2;",
          [](const Model& m) { return isClose(m.lookupPoint("v")->value()(0), 4.); } },

        // scalar starting with a vector primary times a vector
        { "vector component times vector",
          "p=[3,2,1]; v=p.x*[1,0,0];",
          [](const Model& m) { return isClose(m.lookupPoint("v")->value()(0), 3.); } },

        // dot product inside the multiplicative chain
        { "dot product in product",
          "p=[1,2,3]; q=[1,1,1]; s=p&q*2; t=2*p&q;",
          [](const Model& m) {
              return isClose(m.lookupScalar("s")->value(), 12.)
                  && isClose(m.lookupScalar("t")->value(), 12.); } },

        // identifiers starting with inf/nan are no numbers
        { "scalar names starting with inf/nan",
          "infill=3; x=infill*2; nanometer=1e-9; y=nanometer*2;",
          [](const Model& m) {
              return isClose(m.lookupScalar("x")->value(), 6.)
                  && isClose(m.lookupScalar("y")->value(), 2e-9); } },

        // scalar filter argument which starts like a vector
        { "scalar filter argument starting with vector",
          box+"p=[1,0,0]; F=B?faces('minimal(%d0)', p.x);", nullptr },

        // intersection with a provided datum
        { "intersection with provided datum",
          box+"C: Cylinder(O, EX, 1); I: B & C%axis;", nullptr },

        // arithmetic "-" must not swallow feature subtraction
        { "scaled feature minus feature",
          box+"A: Box(O, EX, EY, EZ); D: A*2 - B;", nullptr },

        { "translated feature minus feature",
          box+"A: Box(O, EX, EY, EZ); D: A << [1,0,0] - B;", nullptr },

        // keyword "at" must not match the start of an identifier
        { "provided vertex set name starting with keyword",
          box+"V=B?vertex atTop;", nullptr },

        // selection keyword must be followed by a word boundary,
        // "edgeset" must not be read as "edges et"
        { "selection keyword prefix of identifier",
          box+"E=B?edgeset;", nullptr, true },

        // regular expressions which must keep working
        { "unchanged: scalar arithmetic",
          "a=2; b=a-3*-1; c=-a*b/2+1;",
          [](const Model& m) { return isClose(m.lookupScalar("c")->value(), -4.); } },

        { "unchanged: vector arithmetic",
          "v=2*EX+EY*3-[0,0,1]; w=(v^EX)*0.5;",
          [](const Model& m) {
              arma::mat v=m.lookupPoint("v")->value();
              arma::mat w=m.lookupPoint("w")->value();
              return isClose(v(0),2.) && isClose(v(1),3.) && isClose(v(2),-1.)
                  && isClose(w(1),-0.5) && isClose(w(2),-1.5); } },

        { "unchanged: point in feature CS",
          box+"p=[1,0,0] in B;", nullptr }
    };

    int nFailed=0;
    for (const auto& c: cases)
    {
        std::string result;
        try
        {
            auto m = std::make_shared<cad::Model>();
            int failloc=-1;
            bool ok=parseISCADModel(c.script, m.get(), &failloc);
            if (c.mustFail)
            {
                if (ok) result="script was accepted but should be rejected";
            }
            else if (!ok)
            {
                result="parser stopped at \""+c.script.substr(std::max(0,failloc))+"\"";
            }
            else if (c.check && !c.check(*m))
            {
                result="unexpected value";
            }
        }
        catch (std::exception& e)
        {
            if (!c.mustFail)
                result=std::string("exception: ")+e.what();
        }

        if (result.empty())
        {
            std::cout<<"PASS "<<c.name<<std::endl;
        }
        else
        {
            std::cout<<"FAIL "<<c.name<<": "<<result<<std::endl;
            nFailed++;
        }
    }

    std::cout<<nFailed<<" of "<<cases.size()<<" cases failed."<<std::endl;
    return nFailed>0 ? -1 : 0;
}
