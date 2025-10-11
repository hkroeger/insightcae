#include "parameterlisthash.h"
#include "cadfeature.h"
#include "datum.h"
#include "astbase.h"
#include "subshapenumbering.h"
#include "cadfeature.h"

namespace boost
{


std::size_t hash<TopoDS_Shape>::operator()(const TopoDS_Shape& shape) const
{
    // create hash from
    // 1. total volume
    // 2. # vertices
    // 3. # faces
    // 4. vertex locations

    size_t hash=0;

    GProp_GProps volprops;
    BRepGProp::VolumeProperties(shape, volprops);
    boost::hash_combine(hash, boost::hash<double>()(volprops.Mass()));

    insight::cad::SubshapeNumbering subnum(shape);
    boost::hash_combine(hash, boost::hash<int>()(subnum.nVertexTags()));
    boost::hash_combine(hash, boost::hash<int>()(subnum.nFaceTags()));

    insight::cad::FeatureSetData vset;
    subnum.insertAllVertexTags(vset);
    for (const insight::cad::FeatureID& j: vset)
    {
        auto p=BRep_Tool::Pnt(subnum.vertexByTag(j));
        boost::hash_combine
            (
                hash,
                boost::hash<arma::mat>()(
                    insight::vec3( p.X(), p.Y(), p.Z() ))
                );
    }

    return hash;
}


std::size_t hash<gp_Pnt>::operator()(const gp_Pnt& v) const
{
    std::hash<double> dh;
    size_t h=0;
    boost::hash_combine(h, dh(v.X()));
    boost::hash_combine(h, dh(v.Y()));
    boost::hash_combine(h, dh(v.Z()));
    return h;
}


std::size_t hash<gp_Trsf>::operator()(const gp_Trsf& t) const
{
    std::hash<double> dh;
    size_t h=0;
    for (int c=1; c<=4; c++)
    {
        for (int r=1; r<=3; r++)
        {
            boost::hash_combine(h, dh(t.Value(r, c)));
        }
    }
    boost::hash_combine(h, dh(t.ScaleFactor()));
    return h;
}


std::size_t hash<insight::cad::ASTBase>::operator()(
    const insight::cad::ASTBase& m ) const
{
    return m.hash();
}

std::size_t hash<insight::cad::Feature>::operator()(
    const insight::cad::Feature& m ) const
{
    return hash<insight::cad::ASTBase>()(m);
}

std::size_t hash<insight::cad::FeatureSet>::operator()(
    const insight::cad::FeatureSet& m ) const
{
    return m.calcFeatureSetHash();
}


std::size_t hash<insight::cad::DeferredFeatureSet>::operator()(
    const insight::cad::DeferredFeatureSet& m ) const
{
    return hash<insight::cad::ASTBase>()(m);
}

std::size_t hash<insight::cad::Datum>::operator()(
    const insight::cad::Datum& m ) const
{
    return hash<insight::cad::ASTBase>()(m);
}

std::size_t hash<insight::cad::Scalar>::operator()(
    const insight::cad::Scalar& m ) const
{
    return hash<insight::cad::ASTBase>()(m);
}


std::size_t hash<insight::cad::Vector>::operator()(
    const insight::cad::Vector& m ) const
{
    return hash<insight::cad::ASTBase>()(m);
}


}



namespace insight {
namespace cad {



ParameterListHash::ParameterListHash()
: hash_(0)
{}



size_t ParameterListHash::getHash() const
{
  return hash_;
}



ParameterListHash::operator size_t ()
{
  return hash_;
}


} // namespace cad
} // namespace insight
