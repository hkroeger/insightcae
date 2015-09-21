/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  hannes <email>
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

#include "occtools.h"

namespace insight {
namespace cad {
  
Handle_AIS_InteractiveObject createArrow(const TopoDS_Shape& shape, const std::string& text)
{
  Handle_AIS_RadiusDimension dim=new AIS_RadiusDimension
  (
   shape
#if (OCC_VERSION_MINOR<7)
   , 1e-6, text.c_str()
  );
#else
  );
  dim->SetModelUnits(text.c_str());
#endif
//   Handle_Prs3d_TextAspect ta=dim->Attributes()->TextAspect();
//   ta->SetHeight(100.0);
//   dim->Attributes()->SetTextAspect(ta);
  return dim;
}

Handle_AIS_InteractiveObject createLengthDimension
(
  const TopoDS_Vertex& from, 
  const TopoDS_Vertex& to, 
  const Handle_Geom_Plane& pln,
  double L,
  const std::string& text
)
{
  Handle_AIS_LengthDimension dim(new AIS_LengthDimension(
    from,
    to,
#if (OCC_VERSION_MINOR<7)
    pln,
    L, 
    text.c_str()
  ));
#else
    pln->Pln()
  ));
  dim->SetDisplayUnits(text.c_str());
#endif
  return dim;
}

}
}
