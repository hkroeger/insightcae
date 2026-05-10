#include "bom.h"
#include "cadfeature.h"
#include "parser.h"

namespace insight {
namespace cad {


defineType(BOMCreator);
addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    BOMCreator, &BOMCreator::insertrule );

size_t BOMCreator::calcHash() const
{
    ParameterListHash h;
    if (model_) h+=*model_;
    return h.getHash();
}

BOMCreator::BOMCreator
    (
        FeaturePtr model
        )
    : model_(model)
{}


void BOMCreator::build()
{
    BOM bom;
    model_->addToBOM(bom);
    bom.report(std::cout);
}


void BOMCreator::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.postProcFunctionRules.add
        (
            "BOM",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
              ( "<<" > ruleset.r_solidmodel_expression > ";" )
                [ qi::_val = phx::bind(&BOMCreator::create<FeaturePtr>, qi::_1) ]
            )
        );
}





void BOMCreator::write(std::ostream& ) const
{}



} // namespace cad
} // namespace insight
