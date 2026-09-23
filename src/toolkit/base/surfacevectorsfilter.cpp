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


#include "surfacevectorsfilter.h"

#include "vtkProgrammableFilter.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"

namespace insight
{

namespace {

void ExecuteSurfaceVectors(void *arg)
{
    auto* ctx = static_cast<SurfaceVectorsContext*>(arg);

    auto* input  = vtkPolyData::SafeDownCast(ctx->filter->GetInput());
    auto* output = vtkPolyData::SafeDownCast(ctx->filter->GetOutput());

    // Pass geometry through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());

    // Ensure point normals are available
    vtkSmartPointer<vtkPolyData> withNormals;
    if (!input->GetPointData()->GetNormals())
    {
        auto normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
        normalFilter->SetInputData(input);
        normalFilter->ComputePointNormalsOn();
        normalFilter->ComputeCellNormalsOff();
        normalFilter->SplittingOff(); // preserve topology
        normalFilter->Update();
        withNormals = normalFilter->GetOutput();
    }
    else
    {
        withNormals = input;
    }

    vtkDataArray* normals = withNormals->GetPointData()->GetNormals();
    vtkDataArray* vectors = input->GetPointData()->GetArray(ctx->vectorArrayName.c_str());

    if (!vectors)
    {
        vtkGenericWarningMacro(<< "Array '" << ctx->vectorArrayName << "' not found.");
        return;
    }

    vtkIdType nPts = input->GetNumberOfPoints();

    auto projected = vtkSmartPointer<vtkFloatArray>::New();
    projected->SetName(vectors->GetName());
    projected->SetNumberOfComponents(3);
    projected->SetNumberOfTuples(nPts);

    for (vtkIdType i = 0; i < nPts; ++i)
    {
        double v[3], n[3];
        vectors->GetTuple(i, v);
        normals->GetTuple(i, n);

        // dot product
        double vDotN = v[0]*n[0] + v[1]*n[1] + v[2]*n[2];

        double result[3];
        switch (ctx->mode)
        {
        case SurfaceVectorMode::ParallelToSurface:
            // remove normal component: v - (v·n)n
            result[0] = v[0] - vDotN * n[0];
            result[1] = v[1] - vDotN * n[1];
            result[2] = v[2] - vDotN * n[2];
            break;

        case SurfaceVectorMode::PerpendicularToSurface:
            // keep only normal component: (v·n)n
            result[0] = vDotN * n[0];
            result[1] = vDotN * n[1];
            result[2] = vDotN * n[2];
            break;
        }

        projected->SetTuple(i, result);
    }

    // Replace the array in the output
    output->GetPointData()->RemoveArray(vectors->GetName());
    output->GetPointData()->AddArray(projected);
    output->GetPointData()->SetActiveVectors(projected->GetName());
}

} // anonymous namespace

vtkSmartPointer<vtkProgrammableFilter> MakeSurfaceVectorsFilter(
    vtkPolyData*      input,
    const std::string& vectorArray,
    SurfaceVectorMode  mode)
{
    auto filter = vtkSmartPointer<vtkProgrammableFilter>::New();
    filter->SetInputData(input);

    // Context must outlive the filter's Update() call.
    // Manage lifetime appropriately (e.g. member variable or shared_ptr).
    auto* ctx = new SurfaceVectorsContext{ filter.Get(), vectorArray, mode };
    filter->SetExecuteMethod(ExecuteSurfaceVectors, ctx);

    return filter;
}

}
