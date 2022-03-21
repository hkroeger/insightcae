#include "matplotlibrenderer.h"

#include "base/linearalgebra.h"
#include "base/resultelements/chart.h"
#include "base/boost_include.h"
#include "boost/process.hpp"

#include <iterator>


using namespace std;

#include "base/matplotlibcpp.h"

namespace insight {



MatplotlibRenderer::MatplotlibRenderer(const ChartData *data)
  : ChartRenderer(data)
{}


std::vector<char> colors={'k', 'b', 'g', 'r', 'c', 'm', 'y'};
std::vector<std::string> linestyles={"-", "--", ":", "-."};

char lc(int i)
{
  int ii=i%colors.size();
  return colors[ii];
}

std::string dt(int i)
{
  int ii=i%linestyles.size();
  return linestyles[ii];
}

void MatplotlibRenderer::render(const boost::filesystem::path &outimagepath) const
{
    auto xlabel=boost::replace_all_copy(chartData_->xlabel_, "\\", "\\\\");
    auto ylabel=boost::replace_all_copy(chartData_->ylabel_, "\\", "\\\\");
    {
    std::cout<<"import matplotlib.pyplot as plt;";
    std::cout<<"plt.xlabel('"<<xlabel<<"');";
    std::cout<<"plt.ylabel('"<<ylabel<<"');";
    std::cout<<"plt.grid();";

    int k=0;
    for ( const PlotCurve& pc: chartData_->plc_ )
    {
      ++k;

      int n=pc.xy_.n_rows;
      std::cout<<"plt.plot([";
      for (int i=0; i<n; i++) std::cout << (i>0?",":"") << pc.xy_(i,0);
      std::cout<<"],[";
      for (int i=0; i<n; i++) std::cout << (i>0?",":"") << pc.xy_(i,1);
      std::cout<<"], '";
      std::cout<<lc(pc.style_.color_<0?k:pc.style_.color_)<<dt(pc.style_.dashType_);
      std::cout<<"');";
    }
    std::cout<<"plt.savefig('"<<outimagepath.string()<<"');";
    std::cout<<endl;
  }

  namespace bp = boost::process;
  bp::opstream in;


  bp::child c(
        bp::search_path("python3"),
        bp::std_out > stdout,
        bp::std_in < in
        );

  in<<"import matplotlib.pyplot as plt;";
  in<<"plt.xlabel('"<<xlabel<<"');";
  in<<"plt.ylabel('"<<ylabel<<"');";
  in<<"plt.grid();";

  int k=0;
  for ( const PlotCurve& pc: chartData_->plc_ )
  {
    ++k;

    int n=pc.xy_.n_rows;
    in<<"plt.plot([";
    for (int i=0; i<n; i++) in << (i>0?",":"") << pc.xy_(i,0);
    in<<"],[";
    for (int i=0; i<n; i++) in << (i>0?",":"") << pc.xy_(i,1);
    in<<"], '";
    in<<lc(pc.style_.color_<0?k:pc.style_.color_)<<dt(pc.style_.dashType_);
    in<<"');";
  }
  in<<"plt.savefig('"<<outimagepath.string()<<"');";
  in<<endl;
  in.pipe().close();

  c.wait();
}



} // namespace insight
