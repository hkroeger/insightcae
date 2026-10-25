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
 */

#ifndef IQISCADFOLDINGMARGIN_H
#define IQISCADFOLDINGMARGIN_H

#include <QWidget>

class IQISCADModelScriptEdit;


/**
 * the left margin of the script editor:
 * line numbers and markers for collapsing/expanding foldable ranges.
 * Painting and mouse handling are delegated to the editor.
 */
class IQISCADFoldingMargin
: public QWidget
{
    Q_OBJECT

    IQISCADModelScriptEdit* editor_;

public:
    IQISCADFoldingMargin(IQISCADModelScriptEdit* editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};


#endif // IQISCADFOLDINGMARGIN_H
