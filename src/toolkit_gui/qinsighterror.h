#ifndef QINSIGHTERROR_H
#define QINSIGHTERROR_H

#include "toolkit_gui_export.h"
#include "base/exception.h"
#include "cadfeature.h"
#include "datum.h"

class QWidget;

TOOLKIT_GUI_EXPORT void displayCurrentException(QWidget* parentWidget = nullptr);

#endif // QINSIGHTERROR_H
