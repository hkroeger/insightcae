
#include "solidmodel.h"

using namespace insight;
using namespace insight::cad;

int main()
{
  TopoDS_Shape sh=BRepPrimAPI_MakeCylinder(1.0, 12.0).Shape();
  SolidModel m(sh);
  FeatureSet e = m.query_edges( edgeTopology(edgeTopology::Circle) );
  cout<<e.size()<<endl;
  cout<<*e.begin()<<endl;
  
  return 0;
}