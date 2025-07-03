#ifndef INSIGHT_CADEXCEPTION_H
#define INSIGHT_CADEXCEPTION_H

#include "base/exception.h"
#include "cadfeatures/importsolidmodel.h"
#include "cadtypes.h"
#include <memory>

namespace insight {



struct CADErrorDescription
    : public insight::ErrorDescription // general description
{
public:
    typedef std::map<std::string, cad::FeaturePtr> ContextGeometryMap;

    cad::ConstFeaturePtr geometryInError_;
    ContextGeometryMap contextGeometry_;

    CADErrorDescription(
        const std::string& msg,
        const ContextGeometryMap& contextGeometry = {}
        );

    void saveContextGeometry(const boost::filesystem::path& outFile) const;
};

typedef std::shared_ptr<CADErrorDescription> CADErrorDescriptionPtr;



class CADException
    : public SpecializedException<CADErrorDescription>
{

public:
    template<class ...Args>
    CADException(
        cad::ConstFeaturePtr geometryInError,
        Args&&... addArgs )
        : SpecializedException<CADErrorDescription>(
              std::forward<Args>(addArgs)... )
    {
        description()->geometryInError_=geometryInError;
    }


    template<class ...Args>
    CADException(
        const std::map<std::string, cad::ConstFeaturePtr>& contextGeometry,
        Args&&... addArgs )
        : SpecializedException<CADErrorDescription>(
              std::forward<Args>(addArgs)... )
    {
        for (auto &cg: contextGeometry)
        {
            description()->contextGeometry_.insert(
                {cg.first, std::const_pointer_cast<cad::Feature>(cg.second) } );
        }
    }

    template<class ...Args>
    CADException(
        const std::map<std::string, TopoDS_Shape>& contextGeometry,
        Args&&... addArgs )
        : SpecializedException<CADErrorDescription>(
              std::forward<Args>(addArgs)... )
    {
        for (auto &cg: contextGeometry)
        {
            description()->contextGeometry_.insert(
                {cg.first, cad::Import::create(cg.second) } );
        }
    }
};



class SubElementNotFound
    : public insight::CADException
{
public:
    using CADException::CADException;
};




} // namespace insight

#endif // INSIGHT_CADEXCEPTION_H
