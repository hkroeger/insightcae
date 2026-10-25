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

#ifndef IQISCADCOMPLETER_H
#define IQISCADCOMPLETER_H

#include <QCompleter>
#include <QStandardItemModel>
#include <QColor>

#include <map>
#include <vector>


/**
 * completion of command names, symbols and keywords in the script editor
 */
class IQISCADCompleter
: public QCompleter
{
    Q_OBJECT

public:
    enum CandidateKind
    {
        Command, Function, PostprocCommand,
        Feature, Scalar, Vector, Datum, Selection,
        Keyword
    };

    struct Candidate
    {
        QString name;
        CandidateKind kind;
        /**
         * signature or description, displayed next to the name
         */
        QString detail;
    };

    static const int KindRole = Qt::UserRole+1;

    IQISCADCompleter(QObject* parent);

    /**
     * replace the candidates. Duplicate names are removed (the first one is kept).
     */
    void setCandidates(const std::vector<Candidate>& candidates);

    static QString kindDescription(CandidateKind kind);

private:
    QStandardItemModel* model_;
    std::map<CandidateKind, QIcon> icons_;

    QIcon icon(CandidateKind kind);
};


#endif // IQISCADCOMPLETER_H
