#ifndef SINGLESYMBOLCONSTRAINT_H
#define SINGLESYMBOLCONSTRAINT_H

#include "constrainedsketchentity.h"


namespace insight {
namespace cad {




class SingleSymbolConstraint
    : public ConstrainedSketchEntity
{
public:
    using ConstrainedSketchEntity::ConstrainedSketchEntity;

    virtual std::string symbolText() const =0;
    virtual arma::mat symbolLocation() const =0;

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    bool isInside( SelectionRect r) const override;
};




} // namespace cad
} // namespace insight

#endif // SINGLESYMBOLCONSTRAINT_H
