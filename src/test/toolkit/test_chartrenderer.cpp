
#include "base/linearalgebra.h"
#include "base/resultset.h"

using namespace insight;

int main(int, char*[])
{
  ResultSetPtr res = std::make_shared<ResultSet>(ParameterSet(), "Chart Renderer Test", "");

  arma::mat x = arma::linspace(0, M_PI, 50);
  arma::mat y = sin(pow(x,2));

  addPlot(res, ".", "testChart",
          "$x$", "$\\sin(x^2)$",
          {
            PlotCurve(x, y, "testcurve", PlotCurveStyle().t("tc 1")),
            PlotCurve(x, pow(y,4), "testcurve2", PlotCurveStyle().lc(2).dt(2).y(2).t("tc 2"))
          },
          "test chart"
          );

  res->writeLatexFile("out.tex"); // triggers rendering
}
