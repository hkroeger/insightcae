#include "forcesourcecombination.h"

#include "uniof.h"
#include "multipliedforcesource.h"
#include "subtractedforcesource.h"
#include "constantforcesource.h"

#include "OStringStream.H"
#include "IStringStream.H"

namespace Foam {




void forceSourceCombination::interpretDefinition()
{
    IStringStream is(definition_);

    auto* lastSource = parseSource(is);
    while (!is.eof())
    {
        token operand(is);
        ASSERTION(operand.isWord(), "expected operand!");
        if (operand.wordToken()=="minus")
        {
            auto ns = std::make_shared<subtractedForceSource>(
                        "",
                        lastSource,
                        parseSource(is),
                        false
                        );
            intermediateSources_.push_back(ns);
            lastSource = ns.get();
        }
        else if (operand.wordToken()=="multiplied")
        {
            auto ns = std::make_shared<multipliedForceSource>(
                        "",
                        readScalar(is),
                        lastSource,
                        false
                        );
            intermediateSources_.push_back(ns);
            lastSource = ns.get();
        }
        else
        {
            ERROR("unknown operand: "+operand.wordToken());
        }
    }
    value_=lastSource;
}




forceSource* forceSourceCombination::parseSource(Istream &is)
{
    token typeToken(is);
    ASSERTION(typeToken.isWord(), "expected type identifier!");

    if (typeToken.wordToken()=="constant")
    {
        vector value(is);
        auto ns = std::make_shared<constantForceSource>( "", value, false);
        intermediateSources_.push_back(ns);
        return ns.get();
    }
    else if (typeToken.wordToken()=="lookup")
    {
        token name(is);
        ASSERTION(name.isWord(), "expected name!");
        return forceSources::registry().get(name.wordToken());
    }
    else
    {
        ERROR("unknown force type : "+typeToken.wordToken());
        return nullptr;
    }
}




forceSourceCombination::forceSourceCombination()
: value_(nullptr)
{}




forceSourceCombination::forceSourceCombination(ITstream& is)
    : value_(nullptr)
{
    OStringStream buf;
    unsigned i = 0;
    for (const token& tok : is)
    {
        if (i++)
        {
            buf << ' ';
        }
        buf << tok;
    }
    definition_=buf.str();
}


forceSourceCombination::forceSourceCombination(forceSource *value)
    : value_(value)
{}


vector forceSourceCombination::force() const
{
    if ((value_==nullptr) && !definition_.empty())
    {
        const_cast<forceSourceCombination*>(this)->interpretDefinition();
    }
    ASSERTION(value_!=nullptr, "attempt to use uninitialized force source combination!");
    return value_->force();
}


} // namespace Foam
