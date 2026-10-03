#ifndef BOM_H
#define BOM_H


#include "cadtypes.h"
#include "cadfeature.h"
#include "cadpostprocaction.h"


namespace insight {
namespace cad {

class BOMCreator
    : public PostprocAction
{
    FeaturePtr model_;
    BOM bom_;

    size_t calcHash() const override;
    void build() override;

    BOMCreator(FeaturePtr model);

public:
    declareType("BOMCreator");
    CREATE_FUNCTION(BOMCreator);

    static void insertrule(parser::ISCADParser& ruleset);

    void write(std::ostream& ) const override;
};



} // namespace cad
} // namespace insight

#endif // BOM_H
