#ifndef CHARTRENDERER_H
#define CHARTRENDERER_H

#include <memory>
#include <string>

namespace boost { namespace filesystem { class path; } }

namespace insight
{


struct ChartData;


class ChartRenderer
{
protected:
  const ChartData* chartData_;

public:
  ChartRenderer(const ChartData* data);
  virtual void render(const boost::filesystem::path& outimagepath) const =0;

  static std::unique_ptr<ChartRenderer> create(const ChartData* data);
};


}


#endif // CHARTRENDERER_H
