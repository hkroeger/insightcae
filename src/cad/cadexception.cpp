#include "cadfeatures/importsolidmodel.h"
#include "cadtypes.h"
#include "cadexception.h"
#include "featureset.h"
#include "datum.h"
#include "cadfeatures/compound.h"

#include <thread>

#include "base/translations.h"


namespace insight {





class CADExceptionHandler
    : public ExceptionHandler
{
public:
    CADExceptionHandler() : ExceptionHandler(10000) {}

    ErrorDescriptionPtr describeProblem() const override
    {
        try { throw; }

        catch (insight::CADException& e)
        {
            return e.description();
        }

        catch (Standard_Failure& e)
        {
            auto desc =
                std::make_shared<insight::CADErrorDescription>(
                _("An error has occurred during CAD geometry processing."),
                insight::CADErrorDescription::ContextGeometryMap() );

            desc->errorDetails_= e.GetMessageString();

            return desc;
        }
    }

} theCADExceptionHandler;




CADErrorDescription::CADErrorDescription(
    const std::string &msg,
    const ContextGeometryMap &contextGeometry)
: ErrorDescription(msg),
  contextGeometry_(contextGeometry)
{}



void CADErrorDescription::saveContextGeometry(const boost::filesystem::path &outFile) const
{
    using namespace insight::cad;

    CompoundFeatureMap m;

    std::transform(
        contextGeometry_.begin(),
        contextGeometry_.end(),
        std::inserter(m, m.begin()),
        [&](const ContextGeometryMap::value_type& iv)
        {
            return CompoundFeatureMap::value_type(iv.first, iv.second);
        }
        );

    auto cc=Compound::create(m);

    cc->saveAs(outFile);
}


void CADException::addCG()
{
    if (auto cgf = cg(ExceptionContext::getCurrent()))
    {
        description()->contextGeometry_["context"]=cgf;
    }
}

void CADExceptionContext::setLabel(const std::string& lbl)
{
    label_=lbl;
}

const std::string CADExceptionContext::label() const
{
    return label_;
}

void CADExceptionContext::operator+=(const std::pair<std::string, TopoDS_Shape>& cg)
{
    operator+=({cg.first, cad::Import::create(cg.second)});
}

cad::FeaturePtr CADExceptionContext::contextGeometry() const
{
    return cad::Compound::create(contextGeometry_);
}

void CADExceptionContext::operator+=(const ContextGeometryMap::value_type& cg)
{
    contextGeometry_[cg.first]=cg.second;
}

void CADExceptionContext::operator+=(const ContextGeometryMap& cg)
{
    for(auto &c: cg)
    {
        operator+=(c);
    }
}


cad::FeaturePtr cg(ExceptionContext& ec)
{
    cad::CompoundFeatureMap cf;
    for (const auto& i: boost::adaptors::index(ec))
    {
        if (auto *cec = dynamic_cast<CADExceptionContext*>(i.value()))
        {
            auto label=cec->label();
            if (label.empty())
                label=str(boost::format("context level %d")%i.index());
            cf[label]=cec->contextGeometry();
        }
    }
    return
        cf.size() ?
               cad::Compound::create(cf)
                     : nullptr;
}

} // namespace insight
