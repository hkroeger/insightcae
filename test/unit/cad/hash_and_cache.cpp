
#include "cadfeatures.h"
#include "cadparameters/constantvector.h"
#include "cadparameters/constantscalar.h"
#include "dependencyreplacement.h"

using namespace insight;
using namespace insight::cad;


FeaturePtr unitBox(double lx=1.)
{
    return Box::create(
        matconst(vec3(0,0,0)),
        matconst(vec3(lx,0,0)),
        matconst(vec3(0,1,0)),
        matconst(vec3(0,0,1)) );
}


void testSubshapesOfCompound()
{
    const std::string passiveWallsName="passiveWalls";
    const std::string airExchangeSurfaceName="airExchangeSurface";

    auto geo = cad::Cylinder::create(
        cad::matconst(vec3(0,0,0)),
        cad::matconst(vec3(0,0,1)),
        cad::scalarconst(1),
        false, false );

    auto exf = insight::cad::makeFaceFeatureSet(
        geo, "maximal(area)");
    auto wallf = insight::cad::makeFaceFeatureSet(
        geo, "!in(%0)",
        {exf} );

    auto ex = cad::Import::create(exf);
    auto w = cad::Import::create(wallf);
    std::cout<<"PW_0 ("<<w->hash()<<")"<<std::endl;
    std::cout<<"AES_0 ("<<ex->hash()<<")"<<std::endl;

    auto vp = cad::Compound::create(
        cad::CompoundFeatureMap
        {
            { passiveWallsName,
             cad::Import::create(wallf) },
            { airExchangeSurfaceName,
             cad::Import::create(exf) }
        } );

    auto pwn = vp->subshape(passiveWallsName);
    auto aes = vp->subshape(airExchangeSurfaceName);

    std::cout<<"PW ("<<pwn->hash()<<")"<<std::endl;
    std::cout<<"AES ("<<aes->hash()<<")"<<std::endl;

    insight::assertion(
     !pwn->shape().IsEqual(aes->shape()),
        "both shapes must not be identical!");
}


void testRestoreAcrossGenerations()
{
    cache.clear();

    auto a = unitBox();
    a->checkForBuildDuringAccess();

    // second generation: restored from a
    auto b = unitBox();
    insight::assertion(
        b->shape().IsSame(a->shape()),
        "identical feature was not restored from cache" );

    // third generation, after the first one is gone: restored from b
    auto ashape = a->shape();
    a.reset();
    auto c = unitBox();
    insight::assertion(
        c->shape().IsSame(ashape),
        "cache entry was lost after the original feature was released" );

    // a different feature must not be restored
    auto d = unitBox(2.);
    insight::assertion(
        !d->shape().IsSame(ashape),
        "different feature was restored from cache" );
}


void testInputsAreKeptOnRestore()
{
    cache.clear();

    auto a = unitBox();
    a->setDensity(scalarconst(1000.));
    a->checkForBuildDuringAccess();

    auto b = unitBox();
    b->setDensity(scalarconst(2000.));
    b->checkForBuildDuringAccess();

    insight::assertion(
        b->shape().IsSame(a->shape()),
        "identical feature was not restored from cache" );
    insight::assertion(
        fabs(b->density()-2000.)<1e-10,
        "density was overwritten by the cached feature" );
    insight::assertion(
        fabs(b->mass()-2000.)<1e-6,
        "wrong mass of restored feature" );
}


void testInvalidatedFeatureIsNotReused()
{
    cache.clear();

    auto L1=matconst(vec3(1,0,0));
    auto a = Box::create(
        matconst(vec3(0,0,0)), L1,
        matconst(vec3(0,1,0)),
        matconst(vec3(0,0,1)) );
    a->checkForBuildDuringAccess();
    insight::assertion(
        fabs(a->modelVolume()-1.)<1e-10, "unexpected volume" );

    // change input of a and rebuild it
    a->replaceDependency(
        DependencyReplacement(L1.get(), matconst(vec3(3,0,0))) );
    a->checkForBuildDuringAccess();
    insight::assertion(
        fabs(a->modelVolume()-3.)<1e-10, "feature was not rebuilt after input change" );

    // a fresh feature with the original inputs must not get a's new result
    auto b = unitBox();
    insight::assertion(
        fabs(b->modelVolume()-1.)<1e-10,
        "feature got the result of an invalidated feature from the cache" );
}


void testOptionalInputsAreDistinguished()
{
    auto m = unitBox();
    auto a = matconst(vec3(1,0,0));
    auto b = matconst(vec3(0,0,1));

    auto t1 = Transform::create(m, a, b, ScalarPtr()); // trans=a, rot=b
    auto t2 = Transform::create(m, a, b); // rot=a, rotorg=b

    insight::assertion(
        t1->hash()!=t2->hash(),
        "translation/rotation and rotation/origin must have different hashes" );
}


int main(int argc, char* argv[])
{
    try
    {
        testSubshapesOfCompound();
        testRestoreAcrossGenerations();
        testInputsAreKeptOnRestore();
        testInvalidatedFeatureIsNotReused();
        testOptionalInputsAreDistinguished();
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
