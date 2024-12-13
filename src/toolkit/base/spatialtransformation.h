#ifndef INSIGHT_SPATIALTRANSFORMATION_H
#define INSIGHT_SPATIALTRANSFORMATION_H

#include "base/boost_include.h"
#include "base/linearalgebra.h"

#include <limits>
#include "vtkSmartPointer.h"
//#include "vtkTransform.h"
//#include "vtkPolyDataAlgorithm.h"


class vtkTransform;
class vtkPolyDataAlgorithm;


namespace insight {




class vtk_Transformer
{
public:
  virtual ~vtk_Transformer();
  virtual vtkSmartPointer<vtkPolyDataAlgorithm> apply_VTK_Transform(vtkSmartPointer<vtkPolyDataAlgorithm> in) const =0;
};

typedef vtk_Transformer* vtk_TransformerPtr;
typedef std::vector<const vtk_Transformer*> vtk_TransformerList;




class SpatialTransformation;
class OpenFOAMCase;

#ifndef SWIG
std::ostream& operator<<(std::ostream& os, const SpatialTransformation& st);
#endif


/**
 * @brief The SpatialTransformation class
 * order translate >> rotate >> scale
 */
class SpatialTransformation
        : public vtk_Transformer
{
#ifndef SWIG
    friend std::ostream& operator<<(std::ostream& os, const SpatialTransformation& st);
#endif

protected:
    // apply in this order:

    /**
     * @brief translate_
     * First transformation step: translate point
     */
    arma::mat translate_;

    /**
     * @brief R_
     * Second transformation step: rotation matrix to apply
     */
    arma::mat R_;

    /**
     * @brief scale_
     * Last transformation step: scale
     */
    double scale_;

public:
    /**
     * @brief Transformation
     * identity transform
     */
    SpatialTransformation();

    /**
     * @brief SpatialTransformation
     * construct from components
     * @param translate
     * @param rollPitchYaw
     * angles in degrees!
     * @param scale
     */
    SpatialTransformation(const arma::mat& translate, const arma::mat& rollPitchYaw=vec3(0,0,0), double scale=1.);

    /**
     * @brief SpatialTransformation
     * transformation from local into global CS spanned by ex, ey, ez and with origin at O
     * @param ex
     * @param ey
     * @param ez
     * @param O
     */
    SpatialTransformation(
        const arma::mat& ex,
        const arma::mat& ey,
        const arma::mat& ez,
        const arma::mat& O = vec3Zero()
        );

    /**
     * @brief SpatialTransformation
     * scaled transformation
     * @param scale
     */
    SpatialTransformation(double scale);

    /**
     * @brief SpatialTransformation
     * create a transformation from vtkTransform
     * @param trsf
     */
    SpatialTransformation(vtkTransform* trsf);

    /**
     * @brief setIdentity
     * reset transformation to identy transform
     */
    void setIdentity();

    /**
     * @brief setTranslation
     * change translation part. Rotation and scale remain unchanged.
     * @param translate
     */
    void setTranslation(const arma::mat& translate);

    /**
     * @brief setRotationMatrix
     * change rotation part. Translation and scale remain unchanged.
     * @param R
     */
    void setRotationMatrix(const arma::mat& R);

    /**
     * @brief setRollPitchYaw
     * change rotation part to Euler rotation (order rotation x > y > z). Translation and scale remain unchanged.
     * @param rollPitchYaw
     * angles in degrees!
     */
    void setRollPitchYaw(const arma::mat& rollPitchYaw);

    /**
     * @brief setScale
     * change the scale only. Translation and rotation remain unchanged.
     * @param scale
     */
    void setScale(double scale);

    /**
     * @brief translate
     * return translation part.
     * @return
     */
    const arma::mat& translate() const;

    /**
     * @brief R
     * returns rotation matrix
     * @return
     */
    const arma::mat& R() const;

    /**
     * @brief rollPitchYaw
     * @return Euler angles in degrees.
     */
    arma::mat rollPitchYaw() const;

    /**
     * @brief scale
     * return the scale factor
     * @return
     */
    double scale() const;

    /**
     * @brief rotationMatrix
     * return the rotation matrix.
     * @return
     */
    arma::mat rotationMatrix() const;

    /**
     * @brief trsfPt
     * transform point
     * @param p
     * @return
     */
    arma::mat trsfPt(const arma::mat& p) const;

    /**
     * @brief operator ()
     * transform a point
     * @param p
     * @return
     */
    arma::mat operator()(const arma::mat& p) const;
    arma::mat operator()(double x, double y, double z) const;

    /**
     * @brief trsfVec
     * transform vector (no translation)
     * @param p
     * @return
     */
    arma::mat trsfVec(const arma::mat& p) const;

    /**
     * @brief appendTransformation
     * modify this transformation, so that it yields this transform,
     * followed by the provided, "appended" transformation
     * @param st
     */
    void appendTransformation(const SpatialTransformation& st);

    SpatialTransformation appended(const SpatialTransformation& st) const;

    bool isIdentityTransform() const;

    vtkSmartPointer<vtkTransform> toVTKTransform() const;

    vtkSmartPointer<vtkPolyDataAlgorithm> apply_VTK_Transform(
            vtkSmartPointer<vtkPolyDataAlgorithm> in ) const override;

    void executeOFTransforms(
            const OpenFOAMCase& cm,
            const boost::filesystem::path& exepath,
            const boost::filesystem::path& geofile) const;


    bool operator!=(const SpatialTransformation& o) const;

    void invert();
    SpatialTransformation inverted() const;

};



struct CoordinateSystem
{
    arma::mat origin, ex, ey, ez;

    CoordinateSystem();

    CoordinateSystem(
        const arma::mat& p0,
        const arma::mat& ex );

    CoordinateSystem(
        const arma::mat& p0,
        const arma::mat& ex,
        const arma::mat& ez );

    void rotate(double angle, const arma::mat& axis);

    // arma::mat operator()(double x, double y, double z) const;
    // arma::mat operator()(const arma::mat& pLoc) const;


    SpatialTransformation globalToLocal() const;
    SpatialTransformation localToGlobal() const;

    void setVTKMatrix(vtkMatrix4x4* m);
};



/**
 * @brief The View class
 * Represents the orientation of a view.
 */
struct View : public CoordinateSystem
{
    double cameraDistance;
    std::string title;

    View(
        const arma::mat& ctr,
        const arma::mat& cameraOffset,
        const arma::mat& up,
        const std::string& title );

    inline arma::mat cameraLocation() const { return origin + cameraDistance*ex; }
    inline arma::mat focalPoint() const { return origin; }
    inline arma::mat upwardDirection() const { return ez; }
};



std::map<std::string, View>
generateStandardViews(
    const CoordinateSystem& objectOrientation,
    double cameraDistance );




} // namespace insight

#endif // INSIGHT_SPATIALTRANSFORMATION_H
