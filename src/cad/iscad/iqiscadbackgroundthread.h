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
#include "parser.h"
#endif


#include "iqiscadscriptmodelgenerator.h"
#include "iqiscadmodelrebuilder.h"


class IQISCADSyntaxHighlighter;
class IQISCADMainWindow;


/**
 * the parsing and rebuilding processor.
 * To be started in a separate thread to keep GUI responsive
 */
class IQISCADBackgroundThread
: public QThread
{
    Q_OBJECT


protected:
    IQCADItemModel* model_;
    std::string script_;
    IQISCADScriptModelGenerator::Task finalTask_;
    std::thread::id thread_id_;

public:
//    insight::cad::ModelPtr last_rebuilt_model_, model_;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;

    /**
     * is created upon model construction
     */
    IQISCADBackgroundThread();

    void setModel(IQCADItemModel* model);

    /**
     * restarts the actions
     */
    void launch(const std::string& script, IQISCADScriptModelGenerator::Task executeUntilTask);

    virtual void run();

    inline IQISCADScriptModelGenerator::Task finalTask() const { return finalTask_; }
    void cancelRebuild();

Q_SIGNALS:
    void scriptError(long failpos, QString errorMsg, int range);

    void statusMessage(const QString& msg, double timeout=0);
    void statusProgress(int step, int totalSteps);
    void modelRebuilt();
};



#endif // BGPARSINGTHREAD_H
