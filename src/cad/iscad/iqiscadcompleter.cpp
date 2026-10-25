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

#include "iqiscadcompleter.h"
#include "iqiscadsyntaxhighlighter.h"

#include "base/translations.h"

#include <QTreeView>
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>
#include <QSet>

#include <algorithm>


using namespace insight::cad::parser;


IQISCADCompleter::IQISCADCompleter(QObject* parent)
    : QCompleter(parent),
      model_(new QStandardItemModel(this))
{
    auto *popup = new QTreeView;
    popup->setHeaderHidden(true);
    popup->setRootIsDecorated(false);
    popup->setUniformRowHeights(true);
    popup->setSelectionBehavior(QAbstractItemView::SelectRows);
    popup->setEditTriggers(QAbstractItemView::NoEditTriggers);
    setPopup(popup);

    setModel(model_);
    setCompletionColumn(0);
    setCaseSensitivity(Qt::CaseInsensitive);
    setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    setFilterMode(Qt::MatchStartsWith);
    setWrapAround(false);
    setMaxVisibleItems(12);
}


QString IQISCADCompleter::kindDescription(CandidateKind kind)
{
    switch (kind)
    {
        case Command: return _("feature command");
        case Function: return _("function");
        case PostprocCommand: return _("postprocessing command");
        case Feature: return _("feature");
        case Scalar: return _("scalar");
        case Vector: return _("vector");
        case Datum: return _("datum");
        case Selection: return _("selection");
        case Keyword: return _("keyword");
    }
    return QString();
}


QIcon IQISCADCompleter::icon(CandidateKind kind)
{
    auto i=icons_.find(kind);
    if (i!=icons_.end()) return i->second;

    QColor c;
    QString letter;
    switch (kind)
    {
        case Command:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::FeatureCommand); letter="C"; break;
        case Function:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::ScalarFunction); letter="f"; break;
        case PostprocCommand:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::PostprocCommand); letter="P"; break;
        case Feature:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::FeatureRef); letter="F"; break;
        case Scalar:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::ScalarRef); letter="s"; break;
        case Vector:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::VectorRef); letter="v"; break;
        case Datum:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::DatumRef); letter="D"; break;
        case Selection:
            c=IQISCADSyntaxHighlighter::tokenColor(TokenKind::SelectionRef); letter="S"; break;
        case Keyword:
            c=QColor(128, 0, 128); letter="k"; break;
    }

    const int sz=16;
    QPixmap pm(sz, sz);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(QRectF(1, 1, sz-2, sz-2), 3, 3);
        QFont f=p.font();
        f.setBold(true);
        f.setPixelSize(sz-5);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRect(0, 0, sz, sz), Qt::AlignCenter, letter);
    }
    QIcon ic(pm);
    icons_[kind]=ic;
    return ic;
}


void IQISCADCompleter::setCandidates(const std::vector<Candidate>& candidates)
{
    std::vector<const Candidate*> sorted;
    QSet<QString> names;
    for (const auto& c: candidates)
    {
        if (!c.name.isEmpty() && !names.contains(c.name))
        {
            names.insert(c.name);
            sorted.push_back(&c);
        }
    }
    // the model has to be sorted for CaseInsensitivelySortedModel
    std::sort(sorted.begin(), sorted.end(),
              [](const Candidate* a, const Candidate* b)
              {
                  int r=QString::compare(a->name, b->name, Qt::CaseInsensitive);
                  return r<0 || (r==0 && a->name<b->name);
              });

    model_->clear();
    model_->setColumnCount(2);
    for (const auto* c: sorted)
    {
        auto *name = new QStandardItem(icon(c->kind), c->name);
        name->setData(int(c->kind), KindRole);
        QString detail = c->detail.isEmpty() ? kindDescription(c->kind) : c->detail;
        name->setToolTip(detail);
        auto *det = new QStandardItem(detail);
        det->setForeground(Qt::gray);
        model_->appendRow({name, det});
    }

    if (auto *tv = qobject_cast<QTreeView*>(popup()))
    {
        tv->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        tv->header()->setStretchLastSection(true);
    }
}
