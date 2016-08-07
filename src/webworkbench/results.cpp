#include "results.h"

#include <Wt/WText>

using namespace Wt;

namespace insight
{
namespace web
{
    
Results::Results(SessionPtr session, Wt::WContainerWidget *parent)
: Wt::WContainerWidget(parent),
  session_(session)
{
    create();
}

void Results::create()
{
  new WText("<h2>Results</h2>", this);
}

}
}
