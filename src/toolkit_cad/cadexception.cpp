#include "cadexception.h"
#include "cadtypes.h"

#include <thread>

namespace insight {


void printException_CAD(const std::exception &e)
{
    std::ostringstream title;
    title<<"*** ERROR ["<< std::this_thread::get_id() <<"] ***";

    if (const auto* ie = dynamic_cast<const insight::cad::OCCException*>(&e))
    {
        boost::filesystem::path saveFile = "insightOCCException.brep";
        ie->saveInvolvedShapes(saveFile);

        displayFramed(
                    title.str(),
                    ie->message()+
                    "\nNote: Involved shapes save to file "+saveFile.string()+".",
                    '=', std::cerr);

        if (getenv("INSIGHT_STACKTRACE"))
        {
            std::cerr << "Stack trace:" << std::endl
                      << ie->strace() <<std::endl;
        }
    }
    else
        insight::printException(e);
}

} // namespace insight
