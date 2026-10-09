#ifndef ADDITIONALREFINEMENTS_H
#define ADDITIONALREFINEMENTS_H

#include "snappyhexmesh.h"
#include "additionalrefinements_pdl.h"

namespace insight {

class CADParameterSetModelVisualizer;

class AdditionalRefinement
{
public:
#include ADDITIONALREFINEMENTS_PDL_AdditionalRefinement
/*
PARAMETERSET>>>
inherits insight::snappyHexMeshFeats::RefinementRegion::Parameters

shape = selectablesubset {{

 box set {
   center = vector (0 0 0) "center point" *necessary
   diagonal = vector (1 1 1) "lengths in X/Y/Z direction respectively" *necessary
 }

 cylinder set {
   center = vector (0 0 0) "center point" *necessary
   axis = vector (0 0 0) "axis direction and length (centered around center)" *necessary
   diameter = double 1 "diameter" *necessary
 }

 sphere set {
  center = vector (0 0 0) "Center of the refinement sphere" *necessary
  diameter = double 1 "diameter" *necessary
 }

 geometry
  includedset "insight::ExternalGeometryFile<insight::snappyHexMeshFeats::GeometryOnly>::Parameters"

}} box "select the type of refinement"

createGetter
<<<PARAMETERSET
*/
protected:
    ParameterSetInput p_;
public:
    const ParameterSet& getParameters() const { return p_.parameterSet(); }
    ParameterSetInput getInput() const { return p_.clone(); }

public:
    declareType ( "AdditionalRefinement" );

    AdditionalRefinement(ParameterSetInput ip = Parameters() );

    /**
     * @brief geometry
     * @return
     * CAD representation of the refinement region,
     * null if no valid geometry is available
     */
    cad::FeaturePtr geometry() const;

    /**
     * @brief addToPreview
     * implemented in toolkit_gui (snappyhexmesh_gui.cpp)
     */
    void addToPreview(
        CADParameterSetModelVisualizer &psv,
        gp_Trsf trsf = gp_Trsf() ) const;

    void addIntoSnappyHexMeshConfiguration(
        snappyHexMeshConfiguration::Parameters &p) const;
};

} // namespace insight

#endif // ADDITIONALREFINEMENTS_H
