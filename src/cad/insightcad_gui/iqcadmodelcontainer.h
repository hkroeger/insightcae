#ifndef IQCADMODELCONTAINER_H
#define IQCADMODELCONTAINER_H

#include "insightcad_gui_export.h"

#include <QAbstractItemModel>

class INSIGHTCAD_GUI_EXPORT IQCADModelContainer
    : public QAbstractItemModel
{
  Q_OBJECT

public:
  IQCADModelContainer(QObject* parent=nullptr);
};

#endif // IQCADMODELCONTAINER_H
