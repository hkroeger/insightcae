#include "additionalrefinements.h"

#include "base/linearalgebra.h"
#include "cadfeatures.h"

namespace insight {




defineType(AdditionalRefinement);




AdditionalRefinement::AdditionalRefinement(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}




cad::FeaturePtr AdditionalRefinement::geometry() const
{
    if (const auto* box = boost::get<Parameters::shape_box_type>(&p().shape))
    {
        return cad::Box::create(
            cad::matconst(box->center),
            cad::vec3const(box->diagonal(0), 0, 0),
            cad::vec3const(0, box->diagonal(1), 0),
            cad::vec3const(0, 0, box->diagonal(2)),
            cad::BoxCentering(true, true, true) );
    }
    else if (const auto* cyl = boost::get<Parameters::shape_cylinder_type>(&p().shape))
    {
        return cad::Cylinder::create(
            cad::matconst(cyl->center),
            cad::matconst(cyl->axis),
            cad::scalarconst(cyl->diameter),
            true, true );
    }
    else if (const auto* sph = boost::get<Parameters::shape_sphere_type>(&p().shape))
    {
        return cad::Sphere::create(
            cad::matconst(sph->center),
            cad::scalarconst(sph->diameter) );
    }
    else if (const auto* geo = boost::get<Parameters::shape_geometry_type>(&p().shape))
    {
        if (geo->geometry->isValid())
        {
            cad::FeaturePtr g = cad::Transform::create(
                geo->geometry->geometry(),
                cad::is_gp_Trsf(geo->translate, geo->rollPitchYaw, geo->scalefactor) );

            arma::mat as=geo->inhomscale;
            if ( arma::norm( as-vec3One(), 2 ) > SMALL )
            {
                g=cad::InhomScale::create(g, cad::matconst(as));
            }
            return g;
        }
    }

    return nullptr;
}




void AdditionalRefinement::addIntoSnappyHexMeshConfiguration(
    snappyHexMeshConfiguration::Parameters &p ) const
{
    typedef snappyHexMeshFeats::RefinementRegion::Parameters RRP;

    if (const auto* box = boost::get<Parameters::shape_box_type>(&this->p().shape))
    {
        snappyHexMeshFeats::RefinementBox::Parameters fp;
        static_cast<RRP&>(fp) = this->p();
        fp
            .set_min(box->center - 0.5*box->diagonal)
            .set_max(box->center + 0.5*box->diagonal);

        p.features.push_back(snappyHexMeshFeats::FeaturePtr(
            new snappyHexMeshFeats::RefinementBox(fp) ));
    }
    else if (const auto* cyl = boost::get<Parameters::shape_cylinder_type>(&this->p().shape))
    {
        snappyHexMeshFeats::RefinementCylinder::Parameters fp;
        static_cast<RRP&>(fp) = this->p();
        fp
            .set_point1(cyl->center - 0.5*cyl->axis)
            .set_point2(cyl->center + 0.5*cyl->axis)
            .set_radius(0.5*cyl->diameter);

        p.features.push_back(snappyHexMeshFeats::FeaturePtr(
            new snappyHexMeshFeats::RefinementCylinder(fp) ));
    }
    else if (const auto* sph = boost::get<Parameters::shape_sphere_type>(&this->p().shape))
    {
        snappyHexMeshFeats::RefinementSphere::Parameters fp;
        static_cast<RRP&>(fp) = this->p();
        fp
            .set_center(sph->center)
            .set_radius(0.5*sph->diameter);

        p.features.push_back(snappyHexMeshFeats::FeaturePtr(
            new snappyHexMeshFeats::RefinementSphere(fp) ));
    }
    else if (const auto* geo = boost::get<Parameters::shape_geometry_type>(&this->p().shape))
    {
        snappyHexMeshFeats::RefinementGeometry::Parameters fp;
        static_cast<RRP&>(fp) = this->p();
        fp
            .set_geometry(geo->geometry)
            .set_scalefactor(geo->scalefactor)
            .set_inhomscale(geo->inhomscale)
            .set_translate(geo->translate)
            .set_rollPitchYaw(geo->rollPitchYaw);

        p.features.push_back(snappyHexMeshFeats::FeaturePtr(
            new snappyHexMeshFeats::RefinementGeometry(fp) ));
    }
    else
    {
        throw insight::UnhandledSelection();
    }
}




} // namespace insight
