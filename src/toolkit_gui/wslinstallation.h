#ifndef INSIGHT_WSLINSTALLATION_H
#define INSIGHT_WSLINSTALLATION_H

#include "base/wsllinuxserver.h"

#include <QWidget>

namespace insight {

void checkWSLVersions(bool reportSummary, QWidget *parent=nullptr);

} // namespace insight

#endif // INSIGHT_WSLINSTALLATION_H
