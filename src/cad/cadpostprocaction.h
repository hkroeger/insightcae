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

#ifndef INSIGHT_CAD_POSTPROCACTION_H
#define INSIGHT_CAD_POSTPROCACTION_H

#include "occinclude.h"
#include "astbase.h"

#include "base/factory.h"

#include "vtkSmartPointer.h"
#include "vtkProp.h"



namespace insight
{
namespace cad
{

namespace parser
{
class ISCADParser;
}



/**
 * @brief The PostprocAction class
 * base class of all postprocessing actions.
 *
 * build() shall only perform lightweight evaluations and prepare
 * an easily displayable preview. It is always executed during rebuilds.
 *
 * write() performs the potentially expensive operations
 * (e.g. meshing, file export, reports). It is only executed on request.
 */
class PostprocAction
    : public ASTBase
{
    mutable std::mutex write_mtx_;

public:
    declareType ( "PostprocAction" );

    PostprocAction() = default;
    PostprocAction(const PostprocAction& o);
    PostprocAction& operator=(const PostprocAction& o);

    declareStaticFunctionTable2(
        InsertRule, insertrule,
        void, parser::ISCADParser&);

    /**
     * @brief createVTKRepr
     * creates a visualization of the result in the 3D viewer
     * @return
     * VTK prop
     */
    virtual std::vector<vtkSmartPointer<vtkProp> > createVTKRepr() const;

    /**
     * @brief write
     * export the result. either to console stream (given as parameter)
     * or to some file (e.g. meshing into file).
     * Contains the expensive operations.
     * Requires a previous build, use execute() for automatic build.
     */
    virtual void write(std::ostream& console) const =0;

    /**
     * @brief execute
     * builds (if required) and then writes the result.
     * Concurrent executions of the same action are serialized.
     */
    void execute(std::ostream& console) const;

};

}
}

#endif // INSIGHT_CAD_POSTPROCACTION_H
