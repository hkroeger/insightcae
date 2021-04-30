#ifndef GNUPLOTCHARTRENDERER_H
#define GNUPLOTCHARTRENDERER_H

#include <memory>

#include "base/tools.h"

#include "cpp/poppler-document.h"
#include "cpp/poppler-page.h"
#include "cpp/poppler-page-renderer.h"

#include "gnuplot-iostream.h"

#include "boost/filesystem.hpp"

namespace insight
{


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
    if (getenv("INSIGHT_KEEP_TEMP_DIRS"))
      keep=true;

    CaseDirectory tmp ( keep, bn+"-generate" );

    {
      CurrentExceptionContext ex("executing gnuplot");

      gnuplotio::Gnuplot gp;

      //gp<<"set terminal pngcairo; set termoption dash;";
      gp<<"set terminal epslatex standalone color dash linewidth 3 header \"\\\\usepackage{graphicx}\\n\\\\usepackage{epstopdf}\";";
      gp<<"set output '"+bn+".tex';";
      //     gp<<"set output '"<<absolute(imagepath).string()<<"';";
      /*
              gp<<"set linetype  1 lc rgb '#0000FF' lw 1;"
                  "set linetype  2 lc rgb '#8A2BE2' lw 1;"
                  "set linetype  3 lc rgb '#A52A2A' lw 1;"
                  "set linetype  4 lc rgb '#E9967A' lw 1;"
                  "set linetype  5 lc rgb '#5F9EA0' lw 1;"
                  "set linetype  6 lc rgb '#006400' lw 1;"
                  "set linetype  7 lc rgb '#8B008B' lw 1;"
                  "set linetype  8 lc rgb '#696969' lw 1;"
                  "set linetype  9 lc rgb '#DAA520' lw 1;"
                  "set linetype cycle  9;";
          */

      gnuplotCommand(gp);
    }

    std::system (
          (
            "mv \""+bn+".tex\" \""+ ( tmp/ ( bn+".tex" ) ).string()+"\"; "
            "mv \""+bn+"-inc.eps\" \""+ ( tmp/ ( bn+"-inc.eps" ) ).string()+"\"; "
            "cd \""+tmp.string()+"\"; "
            "pdflatex -interaction=batchmode -shell-escape \""+bn+".tex\"; "
            //"convert -density 600 "+bn+".pdf "+absolute ( imagepath ).string()
            ).c_str() );

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
