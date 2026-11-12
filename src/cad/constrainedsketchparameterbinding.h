#ifndef INSIGHT_CAD_CONSTRAINEDSKETCHPARAMETERBINDING_H
#define INSIGHT_CAD_CONSTRAINEDSKETCHPARAMETERBINDING_H

#include <map>
#include <string>

#include "cadtypes.h"

namespace insight {
namespace cad {

/**
 * @brief The ParameterBinding struct
 * links a (top-level double) parameter of a sketch entity
 * to a scalar expression of the ISCAD script, which contains the sketch.
 */
struct ParameterBinding
{
    std::string expression; ///< expression text, as given in the script
    ScalarPtr value;

    ParameterBinding() = default;
    ParameterBinding(const std::string& expr, ScalarPtr v)
        : expression(expr), value(v)
    {}
};

typedef std::map<std::string, ParameterBinding> ParameterBindings;

}
}

#endif // INSIGHT_CAD_CONSTRAINEDSKETCHPARAMETERBINDING_H
