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

#ifndef BGPARSINGTHREAD_H
#define BGPARSINGTHREAD_H

#include <QThread>
#include <QString>
#include <QMetaType>

#ifndef Q_MOC_RUN
#include "cadmodel.h"
#include "cadtypes.h"
#include "parser.h"
#endif

class ISCADSyntaxHighlighter;
class ISCADMainWindow;


/**
 * the parsing and rebuilding processor.
 * To be started in a separate thread to keep GUI responsive
 */
class BGParsingThread
: public QThread
{
    Q_OBJECT

public:
    enum Action { Parse, Rebuild, Post };

protected:
    std::string script_;
    Action action_;
    std::thread::id thread_id_;

public:
    insight::cad::ModelPtr last_rebuilt_model_, model_;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;

    /**
     * is created upon model construction
     */
    BGParsingThread();

    /**
     * restarts the actions
     */
    void launch(const std::string& script, Action act = Parse);
    virtual void run();
    void extendActionToRebuild();
    inline Action action() const { return action_; }
    void cancelRebuild();

signals:

    void createdVariable    (const QString& sn, insight::cad::ScalarPtr sv);
    void createdVariable    (const QString& sn, insight::cad::VectorPtr vv);
    void createdFeature     (const QString& sn, insight::cad::FeaturePtr sm, bool is_component);
    void createdDatum       (const QString& sn, insight::cad::DatumPtr dm);
    void createdEvaluation  (const QString& sn, insight::cad::PostprocActionPtr em);

    void removedScalar      (const QString& sn);
    void removedVector      (const QString& sn);
    void removedFeature     (const QString& sn);
    void removedDatum       (const QString& sn);
    void removedEvaluation  (const QString& sn);

    void scriptError(long failpos, QString errorMsg, int range);

    void statusMessage(const QString& msg);
    void statusProgress(int step, int totalSteps);
};



#endif // BGPARSINGTHREAD_H
