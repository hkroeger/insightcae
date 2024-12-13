#include "spatialtransformation.h"

#include "vtkTransform.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkTransformPolyDataFilter.h"

#include "openfoam/openfoamcase.h"

namespace insight {


vtk_Transformer::~vtk_Transformer()
{}



std::ostream& operator<<(std::ostream& os, const SpatialTransformation& st)
{
    os << "translate = "<<st.translate().t();
    os << "euler angles = "<<st.rollPitchYaw().t();
    os << "scale = "<<st.scale()<<std::endl;
    return os;
}

SpatialTransformation::SpatialTransformation()
{
    setIdentity();
}

SpatialTransformation::SpatialTransformation(const arma::mat& translate, const arma::mat& rollPitchYaw, double scale)
{
    setTranslation(translate);
    setRollPitchYaw(rollPitchYaw);
    setScale(scale);
}

SpatialTransformation::SpatialTransformation(double scale)
{
    setIdentity();
    setScale(scale);
}

SpatialTransformation::SpatialTransformation(
    const arma::mat& ex,
    const arma::mat& ey,
    const arma::mat& ez,
    const arma::mat& O )
{
    setIdentity();
    R_.col(0)=ex;
    R_.col(1)=ey;
    R_.col(2)=ez;
    translate_ = arma::inv(R_)*O;
}

SpatialTransformation::SpatialTransformation(vtkTransform *t)
{
    double sfs[3], ra[3], tp[3];
    t->GetScale(sfs);
    insight::assertion(
            pow(sfs[1]-sfs[0],2)+pow(sfs[2]-sfs[0],2)<SMALL,
            str(boost::format("the vtkTransform cannot be converted in a SpatialTransformation because the scale factors for the different coordinate directions are not equal (got %g, %g, %g)")
                %sfs[0]%sfs[1]%sfs[2])
            );
    t->GetOrientation(ra);
    t->GetPosition(tp);

    setIdentity();
    setScale(sfs[0]);
    appendTransformation(SpatialTransformation(vec3Zero(), vec3Y(ra[1])));
    appendTransformation(SpatialTransformation(vec3Zero(), vec3X(ra[0])));
    appendTransformation(SpatialTransformation(vec3Zero(), vec3Z(ra[2])));
    appendTransformation(SpatialTransformation(vec3FromComponents(tp)));
}

void SpatialTransformation::setIdentity()
{
    setTranslation(vec3Zero());
    setRollPitchYaw(vec3Zero());
    setScale(1.);
}

void SpatialTransformation::setTranslation(const arma::mat& translate)
{
    translate_=translate;
}

void SpatialTransformation::setRotationMatrix(const arma::mat &R)
{
    R_=R;
}

void SpatialTransformation::setRollPitchYaw(const arma::mat& rollPitchYaw)
{
    R_=rollPitchYawToRotationMatrix(rollPitchYaw);
}

void SpatialTransformation::setScale(double scale)
{
    scale_=scale;
}

const arma::mat& SpatialTransformation::translate() const
{
    return translate_;
}

const arma::mat &SpatialTransformation::R() const
{
    return R_;
}

/**
 * @brief rollPitchYaw
 * @return Euler angles in degrees.
 */
arma::mat SpatialTransformation::rollPitchYaw() const
{
    return rotationMatrixToRollPitchYaw(R_);
}

double SpatialTransformation::scale() const
{
    return scale_;
}


arma::mat SpatialTransformation::rotationMatrix() const
{
    return rollPitchYawToRotationMatrix( rollPitchYaw() );
}

arma::mat SpatialTransformation::trsfPt(const arma::mat& p) const
{
    return scale()* rotationMatrix() * (p+translate());
}


arma::mat SpatialTransformation::operator()(const arma::mat& p) const
{
    return trsfPt(p);
}




arma::mat SpatialTransformation::operator()(double x, double y, double z) const
{
    return trsfPt(vec3(x,y,z));
}


/**
 * @brief trsfVec
 * transform vector (no translation)
 * @param p
 * @return
 */
arma::mat SpatialTransformation::trsfVec(const arma::mat& p) const
{
    return scale()* rotationMatrix() * p;
}


void SpatialTransformation::appendTransformation(const SpatialTransformation &st)
{
    translate_ += arma::inv(scale_*R_) * st.translate();
    scale_ *= st.scale();
    R_ = st.R()*R_;
}

SpatialTransformation SpatialTransformation::appended(const SpatialTransformation &st) const
{
    SpatialTransformation sa(*this);
    sa.appendTransformation(st);
    return sa;
}

bool SpatialTransformation::isIdentityTransform() const
{
    return
            (arma::norm( translate(), 2) < SMALL)
            &&
            (arma::norm( rollPitchYaw(), 2) < SMALL)
            &&
            fabs( scale() -1.) < SMALL
            ;
}


vtkSmartPointer<vtkTransform> SpatialTransformation::toVTKTransform() const
{
  auto t = vtkSmartPointer<vtkTransform>::New();
  t->PostMultiply();
  t->Translate( translate()(0), translate()(1), translate()(2) );
  t->RotateX( rollPitchYaw()(0) );
  t->RotateY( rollPitchYaw()(1) );
  t->RotateZ( rollPitchYaw()(2) );
  t->Scale( scale(), scale(), scale() );
  return t;
}


vtkSmartPointer<vtkPolyDataAlgorithm> SpatialTransformation::apply_VTK_Transform(
        vtkSmartPointer<vtkPolyDataAlgorithm> cad_geom ) const
{
  vtkSmartPointer<vtkPolyDataAlgorithm> res = cad_geom;

  if (!isIdentityTransform())
  {
    auto tf = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    tf->SetInputConnection(cad_geom->GetOutputPort());
    tf->SetTransform( toVTKTransform() );
    res = tf;
  }

  return res;
}

void SpatialTransformation::executeOFTransforms(
        const OpenFOAMCase &cm,
        const boost::filesystem::path &exepath,
        const boost::filesystem::path &geofile ) const
{
    std::string transf_app = geofile.extension()==".eMesh"
          ? "eMeshTransformPoints"
          : "surfaceTransformPoints";

    if (!isIdentityTransform())
    {
          cm.executeCommand(exepath,
            transf_app,
            {
              geofile.string(),
              geofile.string(),
              "-translate", OFDictData::to_OF( translate() ),
              "-rollPitchYaw", OFDictData::to_OF( rollPitchYaw() ),
              "-scale", OFDictData::to_OF( vec3(1,1,1)*scale() )
            }
          );
    }
}


bool SpatialTransformation::operator!=(const SpatialTransformation &o) const
{
    return
            ( arma::norm(o.translate_-translate_,2)>SMALL)
            ||
            ( arma::norm(o.R_-R_,2)>SMALL )
            ||
            (fabs(o.scale_-scale_)>SMALL)
            ;
}

void SpatialTransformation::invert()
{
    translate_ = -scale()*rotationMatrix()*translate_;
    scale_ = 1./scale();
    R_ = arma::inv(R_);
}

SpatialTransformation SpatialTransformation::inverted() const
{
    SpatialTransformation st(*this);
    st.invert();
    return st;
}






CoordinateSystem::CoordinateSystem()
    : origin(vec3Zero()),
    ex(vec3X(1)), ey(vec3Y(1)), ez(vec3Z(1))
{}

CoordinateSystem::CoordinateSystem(const arma::mat &p0, const arma::mat &x)
    : origin(p0),
    ex(x/arma::norm(x,2))
{
    arma::mat tz=vec3(0,0,1);
    if ( fabs(arma::dot(tz,ex) - 1.) < SMALL )
    {
        tz=vec3(0,1,0);
    }

    ey=-arma::cross(ex,tz);
    ey/=arma::norm(ey,2);

    ez=arma::cross(ex,ey);
    ez/=arma::norm(ez,2);
}




CoordinateSystem::CoordinateSystem(const arma::mat &p0, const arma::mat &x, const arma::mat &z)
    : origin(p0),
    ex(x/arma::norm(x,2))
{
    if ( fabs(arma::dot(z,ex) - 1.) < SMALL )
    {
        throw insight::Exception("X and Z axis are colinear!");
    }

    ey=-arma::cross(ex,z);
    ey/=arma::norm(ey,2);

    ez=arma::cross(ex,ey);
    ez/=arma::norm(ez,2);
}

void CoordinateSystem::rotate(double angle, const arma::mat& axis)
{
    arma::mat rot=rotMatrix(angle, axis);
    ex=rot*ex;
    ey=rot*ey;
    ez=rot*ez;
}

SpatialTransformation CoordinateSystem::globalToLocal() const
{
    return localToGlobal().inverted();
}

SpatialTransformation CoordinateSystem::localToGlobal() const
{
    return SpatialTransformation(ex, ey, ez, origin);
}

// arma::mat CoordinateSystem::operator()(double x, double y, double z) const
// {
//     return origin +ex*x +ey*y +ez*z;
// }

// arma::mat CoordinateSystem::operator()(const arma::mat& pLoc) const
// {
//     insight::assertion(
//         pLoc.n_elem==3,
//         "expected vector with 3 components");
//     return operator()(pLoc(0), pLoc(1), pLoc(2));
// }

void CoordinateSystem::setVTKMatrix(vtkMatrix4x4 *m)
{
    m->Identity();
    for (int i=0; i<3; ++i)
    {
        m->SetElement(i, 0, ex(i));
        m->SetElement(i, 1, ey(i));
        m->SetElement(i, 2, ez(i));
        m->SetElement(i, 3, origin(i));
    }
}





View::View(
    const arma::mat &ctr,
    const arma::mat &cameraOffset,
    const arma::mat &up,
    const std::string &t )
    : CoordinateSystem(ctr, cameraOffset, up),
    cameraDistance(arma::norm(cameraOffset, 2)),
    title(t)
{}




std::map<std::string, View>
generateStandardViews(
    const CoordinateSystem &o,
    double camOfs )
{
    return
        {
            {"left",   View(o.origin,  o.ey*camOfs, o.ez, "View from left side") },
            {"right",  View(o.origin, -o.ey*camOfs, o.ez, "View from right side") },
            {"above",  View(o.origin,  o.ez*camOfs, -o.ey, "View from above") },
            {"below",  View(o.origin, -o.ez*camOfs, o.ey, "View from below") },
            {"front",  View(o.origin,  o.ex*camOfs, o.ez, "View from forward") },
            {"aft",    View(o.origin, -o.ex*camOfs, o.ez, "View from aft") },
            {"diag1",  View(o.origin, normalized(  o.ey   +o.ex   +o.ez  )*camOfs, o.ez, "Diagonal view 1: From forward, left, above") },
            {"diag2",  View(o.origin, normalized( -o.ey   +o.ex   +o.ez  )*camOfs, o.ez, "Diagonal view 2: From forward, right, above") },
            {"diag3",  View(o.origin, normalized(  o.ey   +o.ex   -o.ez  )*camOfs, o.ez, "Diagonal view 3: From forward, left, below") },
            {"diag4",  View(o.origin, normalized( -o.ey   +o.ex   -o.ez  )*camOfs, o.ez, "Diagonal view 4: From forward, right, below") },
            {"diag5",  View(o.origin, normalized(  o.ey   -o.ex   +o.ez  )*camOfs, o.ez, "Diagonal view 5: From aft, left, above") },
            {"diag6",  View(o.origin, normalized( -o.ey   -o.ex   +o.ez  )*camOfs, o.ez, "Diagonal view 6: From aft, right, above") },
            {"diag7",  View(o.origin, normalized(  o.ey   -o.ex   -o.ez  )*camOfs, o.ez, "Diagonal view 7: From aft, left, below") },
            {"diag8",  View(o.origin, normalized( -o.ey   -o.ex   -o.ez  )*camOfs, o.ez, "Diagonal view 8: From aft, right, below") }
        };

}





} // namespace insight
