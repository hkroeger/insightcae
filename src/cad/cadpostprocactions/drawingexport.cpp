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

#include "cadfeature.h"
#include "drawingexport.h"
#include "dxfwriter.h"

namespace insight 
{
namespace cad 
{
  
    
    
    
DrawingExport::DrawingExport
(
  const boost::filesystem::path& file,
  std::vector<DrawingViewDefinitions> viewdefs
)
: file_(file),
  viewdefs_(viewdefs)
{}




void DrawingExport::build()
{
    Feature::Views views;
    BOOST_FOREACH(const DrawingViewDefinitions& vds, viewdefs_)
    {
        FeaturePtr model_=boost::fusion::at_c<0>(vds);

        BOOST_FOREACH(const DrawingViewDefinition& vd, boost::fusion::at_c<1>(vds))
        {
            std::string name = boost::get<0>(vd);
            arma::mat p0 = boost::get<1>(vd)->value();
            arma::mat dir = boost::get<2>(vd)->value();
            bool sec = boost::get<4>(vd);
            bool poly = boost::get<5>(vd);
            bool skiphl = boost::get<6>(vd);
            AdditionalViews addviews = boost::get<7>(vd);
            bool left_view = boost::fusion::get<0>(addviews);
            bool right_view = boost::fusion::get<1>(addviews);
            bool top_view = boost::fusion::get<2>(addviews);
            bool bottom_view = boost::fusion::get<3>(addviews);
            bool back_view = boost::fusion::get<4>(addviews);
            if (back_view) left_view=true;

            arma::mat up, right;
            VectorPtr upd=boost::get<3>(vd);
            if (upd)
            {
                up=upd->value();
                right=arma::cross(dir, up);
            }
            

            if (arma::norm(up,2)<1e-6)
                throw insight::Exception("length of upward direction vector must not be zero!");

            views[name] =
                model_->createView
                (
                    p0,
                    dir,
                    sec,
                    up,
                    poly,
                    skiphl
                );
                
            if (left_view)
            {
                std::string thisname = name+"_left";
                
                views[thisname] =
                    model_->createView
                    (
                        p0,
                        -right,
                        false,
                        up,
                        poly,
                        skiphl
                    );
                views[thisname].insert_x = +( 0.55 * views[name].width +0.55 * views[thisname].width );
            }
            if (back_view)
            {
                std::string thisname = name+"_back";
                
                views[thisname] =
                    model_->createView
                    (
                        p0,
                        -dir,
                        false,
                        up,
                        poly,
                        skiphl
                    );
                views[thisname].insert_x = +( 0.55 * views[name].width +1.1 * views[name+"_left"].width +0.55 * views[thisname].width );
            }
            if (right_view)
            {
                std::string thisname = name+"_right";
                
                views[thisname] =
                    model_->createView
                    (
                        p0,
                        right,
                        false,
                        up,
                        poly,
                        skiphl
                    );
                views[thisname].insert_x = -( 0.55 * views[name].width +0.55 * views[thisname].width );
            }
            if (top_view)
            {
                std::string thisname = name+"_top";
                
                views[thisname] =
                    model_->createView
                    (
                        p0,
                        up,
                        false,
                        -dir,
                        poly,
                        skiphl
                    );
                views[thisname].insert_y = -( 0.55 * views[name].height +0.55 * views[thisname].height );
            }
            if (bottom_view)
            {
                std::string thisname = name+"_bottom";
                
                views[thisname] =
                    model_->createView
                    (
                        p0,
                        -up,
                        false,
                        dir,
                        poly,
                        skiphl
                    );
                views[thisname].insert_y = +( 0.55 * views[name].height +0.55 * views[thisname].height );
            }
        }
    }
    shape_=views.begin()->second.visibleEdges;

    DXFWriter::writeViews(file_, views);
}



  
AIS_InteractiveObject* DrawingExport::createAISRepr() const
{
  checkForBuildDuringAccess();
  return new AIS_Shape(shape_);
}




void DrawingExport::write(std::ostream& ) const
{}




}
}
