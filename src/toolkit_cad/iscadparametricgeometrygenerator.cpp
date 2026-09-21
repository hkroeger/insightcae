#include "iscadparametricgeometrygenerator.h"

#include "base/parameters/simpleparameter.h"
#include "cadmodel.h"
#include "cadparameters.h"
#include "cadfeatures/compound.h"

namespace insight {


defineType(ISCADParametricGeometryGenerator);


ISCADParametricGeometryGenerator::ISCADParametricGeometryGenerator(
    rapidxml::xml_node<>& node)
{
    CurrentExceptionContext cex("reading ISCAD parametric model data");


    modelFile_ = SharedPathList::global().getSharedFilePath(
        getMandatoryAttribute<std::string>(node, "ISCADModelFile") );

    auto inputsString = getMandatoryAttribute<std::string>(node, "inputs");
    std::vector<std::string> nvs;
    boost::split(
        nvs,
        inputsString,
        boost::is_any_of(";"),
        boost::algorithm::token_compress_on );

    for (auto& nv: nvs)
    {
        std::vector<std::string> n_v;
        boost::split(
            n_v,
            nv,
            boost::is_any_of("="),
            boost::algorithm::token_compress_on );
        insight::assertion(
            n_v.size()==2,
            "expected <input name>=<default value>, got %s",
            nv.c_str() );

        inputs_[n_v[0]] = toNumber<double>(n_v[1]);
    }
}

std::unique_ptr<ParameterSet>
ISCADParametricGeometryGenerator::defaultParameters() const
{
    auto ps=ParameterSet::create();
    for (auto&v: inputs_)
    {
        ps->insert(
            v.first,
            std::make_unique<DoubleParameter>(
                v.second, "input "+v.first, "", "")
            );
    }
    return ps;
}


cad::FeaturePtr
ISCADParametricGeometryGenerator::generateInstance(
    const ParameterSet& ps ) const
{
    cad::ModelVariableTable vars;
    for (auto&v: inputs_)
    {
        vars.push_back(cad::ModelVariableAndName{
            v.first,
            cad::scalarconst(ps.getDouble(v.first))
        });
    }

    auto model = std::make_shared<cad::Model>(modelFile_, vars);
    model->checkForBuildDuringAccess();

    cad::CompoundFeatureMap components;
    for (auto& c: model->components())
    {
        components[c]=model->lookupModelstep(c);
    }

    return cad::Compound::create(components);
}


} // namespace insight
