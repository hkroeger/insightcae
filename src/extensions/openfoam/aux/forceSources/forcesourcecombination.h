#ifndef FOAM_FORCESOURCECOMBINATION_H
#define FOAM_FORCESOURCECOMBINATION_H

#include "forcesources.h"

#include <vector>
#include <memory>

#include "ITstream.H"


namespace Foam {

class forceSourceCombination
{
    std::string definition_;
    std::vector<std::shared_ptr<forceSource> > intermediateSources_;
    forceSource* value_;

    void interpretDefinition();
    forceSource* parseSource(Istream& is);

public:
    forceSourceCombination();
    forceSourceCombination(ITstream& is);
    forceSourceCombination(forceSource* value);
    vector force() const;
};

} // namespace Foam

#endif // FOAM_FORCESOURCECOMBINATION_H
