#ifndef INSIGHT_CADEXCEPTION_H
#define INSIGHT_CADEXCEPTION_H

#include "base/exception.h"
#include "cadfeatures/importsolidmodel.h"
#include "cadtypes.h"
#include <memory>
#include <string>

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



cad::FeaturePtr cg(ExceptionContext& ec);

class CADException
    : public SpecializedException<CADErrorDescription>
{

    void addCG();

public:
    template<class ...Args>
    CADException(
        cad::ConstFeaturePtr geometryInError,
        Args&&... addArgs )
        : SpecializedException<CADErrorDescription>(
              std::forward<Args>(addArgs)... )
    {
        description()->geometryInError_=geometryInError;
        addCG();
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
        addCG();
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
        addCG();
    }
};



class SubElementNotFound
    : public insight::CADException
{
public:
    using CADException::CADException;
};



class CADExceptionContext
: public CurrentExceptionContext
{
public:
    typedef std::map<std::string, cad::FeaturePtr> ContextGeometryMap;

private:
    std::string label_;
    ContextGeometryMap contextGeometry_;

public:
    using CurrentExceptionContext::CurrentExceptionContext;


    void setLabel(const std::string& lbl);
    void operator+=(const std::pair<std::string, TopoDS_Shape>& cg);
    void operator+=(const ContextGeometryMap::value_type& cg);
    void operator+=(const ContextGeometryMap& cg);

    const std::string label() const;
    cad::FeaturePtr contextGeometry() const;
};


} // namespace insight

#endif // INSIGHT_CADEXCEPTION_H
