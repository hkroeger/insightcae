#ifndef FOAM_FORCESOURCECOMBINATION_H
#define FOAM_FORCESOURCECOMBINATION_H

#include "forcesources.h"

#include <vector>
#include <memory>

#include "fvCFD.H"
#include "ITstream.H"


namespace Foam {

class forceSourceCombination
{
    std::string definition_;
    std::vector<std::shared_ptr<forceSource> > intermediateSources_;
    forceSource* value_;
    const Time* time_;

    void interpretDefinition();
    forceSource* parseSource(Istream& is);

public:
    forceSourceCombination();
    forceSourceCombination(const Time& time, ITstream& is);
    forceSourceCombination(forceSource* value);

    const forceSource* get() const;

    vector force() const;
    scalar torque() const;
};

} // namespace Foam

#endif // FOAM_FORCESOURCECOMBINATION_H
