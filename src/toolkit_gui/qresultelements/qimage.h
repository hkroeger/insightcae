/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef INSIGHT_QIMAGE_H
#define INSIGHT_QIMAGE_H

#include "toolkit_gui_export.h"


#include "iqresultsetmodel.h"

#include <QPixmap>

class QLabel;
class QScrollArea;

namespace insight {

class TOOLKIT_GUI_EXPORT QImage
: public IQResultElement
{
  Q_OBJECT

  int delta_w_;
  QScrollArea* sa_;
  QLabel *id_;
  QPixmap image_;

protected:
  void setImage(const QPixmap& pm);

public:
  declareType ( insight::Image::typeName_() );

  QImage(QObject* parent, const QString& label, insight::ResultElementPtr rep);

  QVariant previewInformation(int role) const override;
  void createFullDisplay(QVBoxLayout *layout) override;
  void resetContents(int width, int height) override;
};

} // namespace insight

#endif // INSIGHT_QIMAGE_H
