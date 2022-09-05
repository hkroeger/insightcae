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

} // namespace insight
