#ifndef INSIGHT_FASTGNUPLOTRENDERER_H
#define INSIGHT_FASTGNUPLOTRENDERER_H

#include <memory>

#include "base/resultelements/chartrenderer.h"
#include "base/resultelements/latexgnuplotrenderer.h"

namespace insight
{



template<class Base>
class FastGnuplotRenderer : public Base
{

protected:
  virtual void gnuplotCommand(gnuplotio::Gnuplot&) const =0;


public:

  template<class ... Types>
  FastGnuplotRenderer(Types ... args)
    : Base(args...)
  {}



  virtual void render(const boost::filesystem::path& outimagepath) const
  {
      using namespace std;
      using namespace boost;
      using namespace poppler;
      using namespace boost::filesystem;

    CurrentExceptionContext ex("rendering chart into image "+outimagepath.string()+" using gnuplot (fast)");

    auto outfn = boost::filesystem::absolute(outimagepath);


    {
      CurrentExceptionContext ex("executing gnuplot");

      auto gp = make_Gnuplot();

      std::string gpfname = outfn.generic_path().string();
      *gp<<"set terminal pngcairo color dash linewidth 3 size 800,600;";
      *gp<<"set output '" << gpfname << "';";
      insight::dbg()<<gpfname<<std::endl;

      gnuplotCommand(*gp);
    }
  }

};




}

#endif // INSIGHT_FASTGNUPLOTRENDERER_H
