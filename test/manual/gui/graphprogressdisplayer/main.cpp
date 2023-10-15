
#include "insightcaeapplication.h"
#include "graphprogressdisplayertestwindow.h"

int main(int argc, char *argv[])
{
  InsightCAEApplication app(argc, argv, "test_graphprogressdisplayer");
  GraphProgressDisplayerTestWindow mw;
  mw.show();
  return app.exec();
}
