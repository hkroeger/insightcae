
#include "cadfeatures.h"
#include "cadparameters/constantvector.h"
#include "cadparameters/constantscalar.h"
#include "base/parameter.h"

#include <iostream>
#include <type_traits>

using namespace insight;
using namespace insight::cad;


// converts implicitly into a constant expression
static_assert(std::is_convertible_v<double, ScalarPtr>);
static_assert(std::is_convertible_v<int, ScalarPtr>);
static_assert(std::is_convertible_v<arma::mat, VectorPtr>);
static_assert(std::is_convertible_v<
              decltype(-(std::declval<arma::mat>()-std::declval<arma::mat>())),
              VectorPtr>);

// values from PDL parameter sets
static_assert(std::is_convertible_v<PrimitiveStaticValueWrap<double>, ScalarPtr>);
static_assert(std::is_convertible_v<const PrimitiveStaticValueWrap<double>&, ScalarPtr>);
static_assert(std::is_convertible_v<PrimitiveStaticValueWrap<int>, ScalarPtr>);
static_assert(std::is_convertible_v<StaticValueWrap<arma::mat>, VectorPtr>);
static_assert(!std::is_convertible_v<PrimitiveStaticValueWrap<bool>, ScalarPtr>);
static_assert(!std::is_convertible_v<PrimitiveStaticValueWrap<double>, VectorPtr>);

// must stay errors
static_assert(!std::is_convertible_v<bool, ScalarPtr>);
static_assert(!std::is_convertible_v<double, VectorPtr>);
static_assert(!std::is_convertible_v<arma::mat, ScalarPtr>);
static_assert(!std::is_convertible_v<ScalarPtr, VectorPtr>);
static_assert(!std::is_convertible_v<VectorPtr, ScalarPtr>);

// pointer semantics are retained
static_assert(std::is_convertible_v<std::shared_ptr<Scalar>, ScalarPtr>);
static_assert(std::is_convertible_v<std::shared_ptr<ConstantScalar>, ScalarPtr>);
static_assert(std::is_convertible_v<ScalarPtr, std::shared_ptr<Scalar> >);
static_assert(std::is_convertible_v<ScalarPtr, ConstScalarPtr>);
static_assert(std::is_convertible_v<std::shared_ptr<ConstantVector>, VectorPtr>);
static_assert(std::is_convertible_v<VectorPtr, ConstVectorPtr>);


void check(bool ok, const std::string& what)
{
    insight::assertion(ok, "check failed: "+what);
}


double acceptScalar(ScalarPtr s)
{
    return s->value();
}


arma::mat acceptVector(VectorPtr v)
{
    return v->value();
}


void testValues()
{
    check(acceptScalar(2.5)==2.5, "double");
    check(acceptScalar(3)==3., "int");
    check(acceptScalar(2.5f)==2.5, "float");

    ScalarPtr zero = 0;
    check(bool(zero), "literal zero must not give a null pointer");
    check(zero->value()==0., "literal zero value");

    arma::mat a=vec3(1,2,3), b=vec3(0,1,1);
    check(arma::approx_equal(
              acceptVector(a), a, "absdiff", 1e-12),
          "arma::mat");
    check(arma::approx_equal(
              acceptVector(-(a-b)), arma::mat(vec3(-1,-1,-2)), "absdiff", 1e-12),
          "arma expression");
}


void testNull()
{
    ScalarPtr s0, s1=nullptr;
    VectorPtr v0, v1=nullptr;
    check(!s0 && !s1, "null ScalarPtr");
    check(!v0 && !v1, "null VectorPtr");
}


void testExpressionIdentity()
{
    auto c = scalarconst(1.);
    ScalarPtr copied = c;
    check(copied.get()==c.get(), "existing scalar expression must not be wrapped");

    auto m = matconst(vec3(1,0,0));
    VectorPtr vcopied = m;
    check(vcopied.get()==m.get(), "existing vector expression must not be wrapped");
}


void testFeatureCreation()
{
    arma::mat ex=vec3(1,0,0), ey=vec3(0,1,0), ez=vec3(0,0,1);

    auto reference = Box::create(
        matconst(vec3(0,0,0)), matconst(ex), matconst(ey), matconst(ez) );

    auto box = Box::create( arma::mat(vec3(0,0,0)), ex, ey, ez );
    check(box->hash()==reference->hash(), "converted arguments must yield the same hash");

    auto expressionBox = Box::create( ex-ex, 2.*ex-ex, ey, ez );
    check(expressionBox->hash()==reference->hash(),
          "expression arguments must yield the same hash");

    // Extrusion is overloaded for ScalarPtr and VectorPtr
    auto face = insight::cad::makeFaceFeatureSet(reference, "maximal(CoG.z)");
    auto section = Import::create(face);

    auto byVector = Extrusion::create(section, matconst(ez));
    auto byMat = Extrusion::create(section, ez);
    check(byMat->hash()==byVector->hash(), "extrusion along arma::mat");

    auto byVectorExpr = Extrusion::create(section, 2.*ez-ez);
    check(byVectorExpr->hash()==byVector->hash(), "extrusion along arma expression");

    auto byScalar = Extrusion::create(section, scalarconst(1.));
    auto byDouble = Extrusion::create(section, 1.);
    check(byDouble->hash()==byScalar->hash(), "extrusion by double");
    check(byDouble->hash()!=byVector->hash(), "scalar and vector overloads differ");

    const PrimitiveStaticValueWrap<double> wrappedLength(1.);
    check(acceptScalar(wrappedLength)==1., "wrapped double");
    auto byWrapped = Extrusion::create(section, wrappedLength);
    check(byWrapped->hash()==byScalar->hash(), "extrusion by wrapped double");

    const StaticValueWrap<arma::mat> wrappedDirection(ez);
    check(arma::approx_equal(acceptVector(wrappedDirection), ez, "absdiff", 1e-12),
          "wrapped arma::mat");
    auto byWrappedVector = Extrusion::create(section, wrappedDirection);
    check(byWrappedVector->hash()==byVector->hash(), "extrusion along wrapped arma::mat");
}


int main(int, char*[])
{
    try
    {
        testValues();
        testNull();
        testExpressionIdentity();
        testFeatureCreation();
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }
    return 0;
}
