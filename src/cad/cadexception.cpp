#include "cadexception.h"
#include "cadtypes.h"

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



} // namespace insight
