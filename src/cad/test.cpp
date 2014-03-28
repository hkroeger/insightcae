#include "boost/foreach.hpp"
#include "base/linearalgebra.h"
#include "solidmodel.h"

using namespace insight;
using namespace insight::cad;

int main()
{
  arma::mat p0=vec3(0,0,0);
  arma::mat ax=vec3(0,0,1), ax2=vec3(0,1,0);

  Box b(p0-4.5*ax, vec3(0,0.2,0.7), vec3(0,-0.3,0.7), vec3(0.7,0.2,0));
  SolidModel m=
    (Cylinder(p0-5.*ax, p0+5.*ax, 1) 
    | 
    Sphere(p0+5.*ax, 1)
    |
    Cylinder(p0-5.0*ax2, p0+5.*ax2, 1.1)
    )-
    b
    ;
    
  m.saveAs( "shape.brep" );
  
  FeatureSet e;
  
  e = m.query_edges( coincides(b, b.query_edges(everything())) );
  cout<<e<<endl;
  
//   
//   e = m.query_edges( edgeTopology(GeomAbs_Circle) );
//   cout<<e<<endl;
//   
//   e = m.query_edges
//   ( 
//     edgeTopology(GeomAbs_Circle) 
//     && 
//     ( as_scalar<arma::mat>( arma::mat(ax.t()) * edgeCoG() ) > 0.0) 
//   );
//   cout<<e<<endl;
//   
//   e = m.query_edges( !edgeTopology(GeomAbs_Circle) && !edgeTopology(GeomAbs_Line) );
//   cout<<e<<endl;
  
  return 0;
}