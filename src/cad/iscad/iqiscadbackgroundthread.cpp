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

#include "iqiscadbackgroundthread.h"

#include "cadfeature.h"
#include "cadmodel.h"

#include <thread>


IQISCADBackgroundThread::IQISCADBackgroundThread()
: finalTask_(IQISCADScriptModelGenerator::Parse),
  model_(nullptr)
{
}

void IQISCADBackgroundThread::setModel(IQCADItemModel *model)
{
    model_=model;
}




void IQISCADBackgroundThread::launch(const std::string& script, IQISCADScriptModelGenerator::Task executeUntilTask)
{
    script_=script;
    finalTask_=executeUntilTask;
    start();
}





void IQISCADBackgroundThread::run()
{
    thread_id_=std::this_thread::get_id();

    IQISCADScriptModelGenerator mgen;
    connect( &mgen, &IQISCADScriptModelGenerator::scriptError,
             this, &IQISCADBackgroundThread::scriptError );
    connect( &mgen, &IQISCADScriptModelGenerator::statusMessage,
             this, &IQISCADBackgroundThread::statusMessage );
    connect( &mgen, &IQISCADScriptModelGenerator::statusProgress,
             this, &IQISCADBackgroundThread::statusProgress );

    std::unique_ptr<IQISCADModelRebuilder> mrb;
    if (model_ && finalTask_>=IQISCADScriptModelGenerator::Rebuild)
    {
        mrb.reset(new IQISCADModelRebuilder(model_, {&mgen}));
    }

    syn_elem_dir_ = mgen.generate(script_, finalTask_);
}


void IQISCADBackgroundThread::cancelRebuild()
{
  insight::cad::Feature::cancelRebuild(thread_id_);
}
