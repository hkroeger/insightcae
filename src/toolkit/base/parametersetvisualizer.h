#ifndef INSIGHT_PARAMETERSETVISUALIZER_H
#define INSIGHT_PARAMETERSETVISUALIZER_H

#include "base/progressdisplayer/textprogressdisplayer.h"

#include <armadillo>

class QIcon;

namespace insight {

struct CameraState
{
    bool isParallelProjection;
    double parallelScale;
    arma::mat cameraPosition, focalPosition, viewUp;
};

class ParameterSet;

class ParameterSetVisualizer
{

  // not linked to CAD; don't use any non-forward definitions from CAD module
private:
    TextProgressDisplayer defaultProgressDisplayer_;

    const ParameterSet *scheduledParameters_;
    std::unique_ptr<ParameterSet> visualizedParameters_;

protected:
    ProgressDisplayer* progress_;

public:
    virtual bool hasScheduledParameters() const;
    virtual bool hasCurrentParameters() const;
    virtual const ParameterSet& currentParameters() const;
    virtual bool selectScheduledParameters();
    virtual void clearScheduledParameters();

    ParameterSetVisualizer();
    virtual ~ParameterSetVisualizer();

    /**
     * @brief update
     * @param ps
     * updates the parameterset which is to visualize.
     * This triggers recomputation of visualization features (from insight::cad) for several parameters.
     */
    virtual void update(const ParameterSet& ps);

    virtual void setIcon(QIcon* icon);

    void setProgressDisplayer(ProgressDisplayer* pd);

    virtual CameraState defaultCameraState() const;
};

typedef std::shared_ptr<ParameterSetVisualizer> ParameterSetVisualizerPtr;



} // namespace insight

#endif // INSIGHT_PARAMETERSETVISUALIZER_H
