#ifndef INSIGHT_SPATIALTRANSFORMATION_H
#define INSIGHT_SPATIALTRANSFORMATION_H

#include "base/boost_include.h"
#include "base/linearalgebra.h"

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

std::ostream& operator<<(std::ostream& os, const SpatialTransformation& st);

class SpatialTransformation
        : public vtk_Transformer
{
    friend std::ostream& operator<<(std::ostream& os, const SpatialTransformation& st);

protected:
    // apply in this order:
    arma::mat translate_;

    /**
     * @brief R_
     * rotation matrix
     */
    arma::mat R_;

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

    void setTranslation(const arma::mat& translate);

    /**
     * @brief setRollPitchYaw
     * @param rollPitchYaw
     * angles in degrees!
     */
    void setRollPitchYaw(const arma::mat& rollPitchYaw);
    void setScale(double scale);

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
    double scale() const;


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
     * transform point
     * @param p
     * @return
     */
    arma::mat operator()(const arma::mat& p) const;

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
};




} // namespace insight

#endif // INSIGHT_SPATIALTRANSFORMATION_H
