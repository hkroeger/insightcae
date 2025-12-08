#ifndef PARAMETERLISTHASH_H
#define PARAMETERLISTHASH_H

#include "base/cacheableentityhashes.h"

#include "cadtypes.h"

namespace boost
{


template<> struct hash<TopoDS_Shape>
{
    std::size_t operator()(const TopoDS_Shape& shape) const;
};

template<> struct hash<gp_Pnt>
{
    std::size_t operator()(const gp_Pnt& v) const;
};

template<> struct hash<gp_Trsf>
{
    std::size_t operator()(const gp_Trsf& v) const;
};

template<> struct hash<insight::cad::ASTBase>
{
    std::size_t operator()(const insight::cad::ASTBase& m) const;
};

template<> struct hash<insight::cad::Feature>
{
    std::size_t operator()(const insight::cad::Feature& m) const;
};

template<> struct hash<insight::cad::FeatureSet>
{
    std::size_t operator()(const insight::cad::FeatureSet& m) const;
};

template<> struct hash<insight::cad::DeferredFeatureSet>
{
    std::size_t operator()(const insight::cad::DeferredFeatureSet& m) const;
};

template<> struct hash<insight::cad::Datum>
{
    std::size_t operator()(const insight::cad::Datum& m) const;
};

template<> struct hash<insight::cad::Scalar>
{
    std::size_t operator()(const insight::cad::Scalar& m) const;
};

template<> struct hash<insight::cad::Vector>
{
    std::size_t operator()(const insight::cad::Vector& m) const;
};



}



namespace insight {
namespace cad {



class ParameterListHash
{
    size_t hash_;

public:
    ParameterListHash();

    template<class T>
    void addParameter(const T& p)
    {
        boost::hash_combine<T>(hash_, p);
    }

    template<class T>
    void operator+=(const T& p)
    {
        addParameter(p);
    }

    operator size_t ();

    size_t getHash() const;
};

} // namespace cad
} // namespace insight

#endif // PARAMETERLISTHASH_H
