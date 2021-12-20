#ifndef GNUPLOTCHARTRENDERER_H
#define GNUPLOTCHARTRENDERER_H

#include <memory>

#include "base/tools.h"
#include "base/casedirectory.h"
#include "base/externalprograms.h"

#include "cpp/poppler-document.h"
#include "cpp/poppler-page.h"
#include "cpp/poppler-page-renderer.h"

#include "gnuplot-iostream.h"

#include "boost/filesystem.hpp"

namespace insight
{


std::unique_ptr<gnuplotio::Gnuplot> make_Gnuplot();


template<class Base>
class GnuplotRendererBase : public Base
{


protected:

  virtual void gnuplotCommand(gnuplotio::Gnuplot&) const =0;



public:

  template<class ... Types>
  GnuplotRendererBase(Types ... args)
    : Base(args...)
  {}



  virtual void render(const boost::filesystem::path& outimagepath) const
  {
      using namespace std;
      using namespace boost;
      using namespace poppler;
      using namespace boost::filesystem;

    CurrentExceptionContext ex("rendering chart into image "+outimagepath.string()+" usign gnuplot");

    string bn ( outimagepath.filename().stem().string() );

    bool keep=false;
    if (getenv("INSIGHT_KEEPTEMPDIRS"))
      keep=true;

    CaseDirectory tmp ( keep, bn+"-generate" );

    {
      CurrentExceptionContext ex("executing gnuplot");

      auto gp = make_Gnuplot();

      std::string gpfname = (tmp/(bn+".tex")).generic_path().string();
      *gp<<"set terminal cairolatex pdf standalone color dash linewidth 3;";
      *gp<<"set output '" << gpfname << "';";
      insight::dbg()<<gpfname<<std::endl;

      gnuplotCommand(*gp);
    }

    boost::process::system(
          boost::process::search_path("pdflatex"),
          boost::process::args(
            { "-interaction=batchmode", "-shell-escape", bn+".tex" }),
          boost::process::start_dir(tmp)
          );

    std::shared_ptr<document> doc( document::load_from_file( (tmp/(bn+".pdf")).string() ) );
    if (!doc) {
      throw insight::Exception("loading error");
    }

    if (doc->is_locked()) {
      throw insight::Exception("pdflatex produced encrypted document");
    }

    if (doc->pages()!=1)
      throw insight::Exception(str(format("expected one single page in chart PDF, got %d!")%doc->pages()));

    std::shared_ptr<poppler::page> page(doc->create_page(0));
    if (!page) {
      throw insight::Exception("could not extract page from PDF document");
    }
    poppler::page_renderer pr;
    pr.set_render_hint(poppler::page_renderer::antialiasing, true);
    pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);

    poppler::image img = pr.render_page(page.get(), 600, 600);
    if (!img.is_valid()) {
      throw insight::Exception("rendering failed");
    }

    if (!img.save( boost::filesystem::absolute(outimagepath).string(), "png", 600)) {
      throw insight::Exception("saving to file failed");
    }
  }

};



}



#endif // GNUPLOTCHARTRENDERER_H
