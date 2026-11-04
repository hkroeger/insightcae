#ifndef INSIGHT_FRAMEDVECTOR_H
#define INSIGHT_FRAMEDVECTOR_H

#include <ostream>
#include <type_traits>

#include "base/exception.h"
#include "base/linearalgebra.h"
#include "base/spatialtransformation.h"


/**
 * Points and vectors tagged with the frame of reference (coordinate system),
 * in which their components are expressed.
 *
 * The frame is a compile-time tag (similar to units in boost::units):
 * combining quantities from different frames does not compile.
 * Changing the frame is only possible through a FrameTransformation<From,To>,
 * which wraps a SpatialTransformation. Points are transformed with
 * translation, vectors (directions, displacements) without.
 *
 * The tags only name the role of a frame in the code. The actual (runtime)
 * transformation is bound to the tags once, e.g. by localToParent<Local>(cs).
 *
 * This is an additive layer: the untagged arma::mat API of
 * SpatialTransformation and CoordinateSystem remains unchanged.
 * Use the explicit constructors and raw() to interface with untagged code.
 *
 * Example:
 * @code
 * INSIGHT_DECLARE_FRAME(BeamLocal);
 * auto toGlobal = localToParent<BeamLocal>(beamCS);
 * PointIn<BeamLocal> pl(0, 0, L);
 * PointIn<GlobalFrame> pg = toGlobal(pl);
 * VectorIn<GlobalFrame> d = pg - originOf<GlobalFrame>(beamCS); // ok
 * // pg - pl; // does not compile: different frames
 * @endcode
 *
 * Note: a SpatialTransformation with scale factor != 1 also scales vectors,
 * i.e. unit vectors do not remain unit vectors.
 */


namespace insight {


/**
 * @brief The GlobalFrame struct
 * tag for the global frame of reference
 */
struct GlobalFrame { static constexpr const char* name = "global"; };


/**
 * declare a frame tag. Use at namespace scope.
 */
#define INSIGHT_DECLARE_FRAME(Tag) \
  struct Tag { static constexpr const char* name = #Tag; }


namespace framedvector_detail {

template<class...> struct always_false : std::false_type {};

inline arma::mat checkedVec3(const arma::mat& v)
{
    insight::assertion(
        v.n_elem==3,
        "expected a vector with 3 components" );
    return arma::vectorise(v);
}

}


#define INSIGHT_FRAME_MISMATCH_MESSAGE \
  "points/vectors are expressed in different frames - convert with FrameTransformation<From,To> first"




/**
 * @brief The VectorIn class
 * direction or displacement, expressed in frame F.
 * Not affected by the translational part of a transformation.
 */
template<class F>
class VectorIn
{
    arma::mat v_;

public:
    typedef F Frame;

    VectorIn() : v_(vec3Zero()) {}
    VectorIn(double x, double y, double z) : v_(vec3(x, y, z)) {}

    /**
     * tag untagged components as being expressed in frame F
     */
    explicit VectorIn(const arma::mat& v)
        : v_(framedvector_detail::checkedVec3(v)) {}

    /**
     * the untagged components (3x1)
     */
    const arma::mat& raw() const { return v_; }

    double operator()(arma::uword i) const { return v_(i); }
    double x() const { return v_(0); }
    double y() const { return v_(1); }
    double z() const { return v_(2); }

    VectorIn& operator+=(const VectorIn& o) { v_+=o.v_; return *this; }
    VectorIn& operator-=(const VectorIn& o) { v_-=o.v_; return *this; }
    VectorIn& operator*=(double s) { v_*=s; return *this; }
    VectorIn& operator/=(double s) { v_/=s; return *this; }
};




/**
 * @brief The PointIn class
 * location, expressed in frame F.
 */
template<class F>
class PointIn
{
    arma::mat p_;

public:
    typedef F Frame;

    PointIn() : p_(vec3Zero()) {}
    PointIn(double x, double y, double z) : p_(vec3(x, y, z)) {}

    /**
     * tag untagged coordinates as being expressed in frame F
     */
    explicit PointIn(const arma::mat& p)
        : p_(framedvector_detail::checkedVec3(p)) {}

    /**
     * the untagged coordinates (3x1)
     */
    const arma::mat& raw() const { return p_; }

    double operator()(arma::uword i) const { return p_(i); }
    double x() const { return p_(0); }
    double y() const { return p_(1); }
    double z() const { return p_(2); }

    PointIn& operator+=(const VectorIn<F>& v) { p_+=v.raw(); return *this; }
    PointIn& operator-=(const VectorIn<F>& v) { p_-=v.raw(); return *this; }
};




// ===== vector algebra

template<class F> VectorIn<F> operator+(VectorIn<F> a, const VectorIn<F>& b) { return a+=b; }
template<class F> VectorIn<F> operator-(VectorIn<F> a, const VectorIn<F>& b) { return a-=b; }
template<class F> VectorIn<F> operator-(const VectorIn<F>& a) { return VectorIn<F>(-a.raw()); }
template<class F> VectorIn<F> operator*(double s, VectorIn<F> a) { return a*=s; }
template<class F> VectorIn<F> operator*(VectorIn<F> a, double s) { return a*=s; }
template<class F> VectorIn<F> operator/(VectorIn<F> a, double s) { return a/=s; }

template<class F> double dot(const VectorIn<F>& a, const VectorIn<F>& b)
{ return arma::dot(a.raw(), b.raw()); }

template<class F> VectorIn<F> cross(const VectorIn<F>& a, const VectorIn<F>& b)
{ return VectorIn<F>(arma::cross(a.raw(), b.raw())); }

template<class F> double norm(const VectorIn<F>& a)
{ return arma::norm(a.raw(), 2); }

template<class F> VectorIn<F> normalized(const VectorIn<F>& a)
{ return VectorIn<F>(insight::normalized(a.raw())); }


// ===== affine point algebra

template<class F> VectorIn<F> operator-(const PointIn<F>& a, const PointIn<F>& b)
{ return VectorIn<F>(a.raw()-b.raw()); }

template<class F> PointIn<F> operator+(PointIn<F> p, const VectorIn<F>& v) { return p+=v; }
template<class F> PointIn<F> operator+(const VectorIn<F>& v, PointIn<F> p) { return p+=v; }
template<class F> PointIn<F> operator-(PointIn<F> p, const VectorIn<F>& v) { return p-=v; }

template<class F> double distance(const PointIn<F>& a, const PointIn<F>& b)
{ return norm(a-b); }


// ===== catch-alls for misuse: give readable compile errors.
// The same-frame overloads above are more specialized and win, if applicable.

template<class A, class B> void operator+(const VectorIn<A>&, const VectorIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }
template<class A, class B> void operator-(const VectorIn<A>&, const VectorIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }
template<class A, class B> void dot(const VectorIn<A>&, const VectorIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }
template<class A, class B> void cross(const VectorIn<A>&, const VectorIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }

template<class A, class B> void operator-(const PointIn<A>&, const PointIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }
template<class A, class B> void operator+(const PointIn<A>&, const VectorIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }
template<class A, class B> void operator+(const VectorIn<A>&, const PointIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }
template<class A, class B> void operator-(const PointIn<A>&, const VectorIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }
template<class A, class B> void distance(const PointIn<A>&, const PointIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value, INSIGHT_FRAME_MISMATCH_MESSAGE); }

template<class A, class B> void operator+(const PointIn<A>&, const PointIn<B>&)
{ static_assert(framedvector_detail::always_false<A,B>::value,
                "adding two points is not meaningful - subtract them to get a vector or add a vector to a point"); }
template<class F> void operator*(double, const PointIn<F>&)
{ static_assert(framedvector_detail::always_false<F>::value,
                "scaling a point is not meaningful - scale a vector relative to some reference point instead"); }
template<class F> void operator*(const PointIn<F>&, double)
{ static_assert(framedvector_detail::always_false<F>::value,
                "scaling a point is not meaningful - scale a vector relative to some reference point instead"); }




/**
 * @brief The FrameTransformation class
 * maps points and vectors expressed in frame From into frame To.
 */
template<class From, class To>
class FrameTransformation
{
    SpatialTransformation t_;

public:
    typedef From FromFrame;
    typedef To ToFrame;

    explicit FrameTransformation(const SpatialTransformation& t = SpatialTransformation())
        : t_(t) {}

    template<class G>
    PointIn<To> operator()(const PointIn<G>& p) const
    {
        static_assert(std::is_same<G,From>::value,
                      "point is not expressed in the source frame of this transformation");
        return PointIn<To>(t_.trsfPt(p.raw()));
    }

    template<class G>
    VectorIn<To> operator()(const VectorIn<G>& v) const
    {
        static_assert(std::is_same<G,From>::value,
                      "vector is not expressed in the source frame of this transformation");
        return VectorIn<To>(t_.trsfVec(v.raw()));
    }

    FrameTransformation<To,From> inverted() const
    {
        return FrameTransformation<To,From>(t_.inverted());
    }

    /**
     * the untagged transformation
     */
    const SpatialTransformation& spatialTransformation() const { return t_; }
};


/**
 * composition: first apply tAB, then tBC
 */
template<class A, class B, class C>
FrameTransformation<A,C> operator*(
    const FrameTransformation<B,C>& tBC,
    const FrameTransformation<A,B>& tAB )
{
    return FrameTransformation<A,C>(
        tBC.spatialTransformation() * tAB.spatialTransformation() );
}

template<class A, class B, class C, class D>
void operator*(
    const FrameTransformation<C,D>&,
    const FrameTransformation<A,B>& )
{
    static_assert(framedvector_detail::always_false<A,B,C,D>::value,
                  "cannot compose transformations: target frame of the right operand differs from source frame of the left operand");
}




// ===== bridge to the untagged CoordinateSystem

/**
 * transformation from the coordinate system cs (frame Local)
 * into the frame (Parent), in which cs itself is defined.
 */
template<class Local, class Parent = GlobalFrame>
FrameTransformation<Local,Parent> localToParent(const CoordinateSystem& csInParent)
{
    return FrameTransformation<Local,Parent>(csInParent.localToGlobal());
}

/**
 * transformation from the frame (Parent), in which cs is defined,
 * into the coordinate system cs (frame Local).
 */
template<class Local, class Parent = GlobalFrame>
FrameTransformation<Parent,Local> parentToLocal(const CoordinateSystem& csInParent)
{
    return FrameTransformation<Parent,Local>(csInParent.globalToLocal());
}

template<class Parent> PointIn<Parent> originOf(const CoordinateSystem& cs)
{ return PointIn<Parent>(cs.origin); }

template<class Parent> VectorIn<Parent> exOf(const CoordinateSystem& cs)
{ return VectorIn<Parent>(cs.ex); }

template<class Parent> VectorIn<Parent> eyOf(const CoordinateSystem& cs)
{ return VectorIn<Parent>(cs.ey); }

template<class Parent> VectorIn<Parent> ezOf(const CoordinateSystem& cs)
{ return VectorIn<Parent>(cs.ez); }




// ===== output

template<class F>
std::ostream& operator<<(std::ostream& os, const PointIn<F>& p)
{
    return os<<"point("<<p.x()<<" "<<p.y()<<" "<<p.z()<<")@"<<F::name;
}

template<class F>
std::ostream& operator<<(std::ostream& os, const VectorIn<F>& v)
{
    return os<<"vector("<<v.x()<<" "<<v.y()<<" "<<v.z()<<")@"<<F::name;
}


} // namespace insight

#endif // INSIGHT_FRAMEDVECTOR_H
