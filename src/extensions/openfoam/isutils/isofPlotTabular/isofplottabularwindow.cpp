

#include "isofplottabularwindow.h"
#include "plotwidget.h"

#include "base/exception.h"

#include "boost/algorithm/string/trim.hpp"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

IsofPlotTabularWindow::IsofPlotTabularWindow(const boost::filesystem::path& file)
  : QMainWindow(),
    file_(file)
{
  ui=new Ui_MainWindow;
  ui->setupUi(this);

  onUpdate();
}


void IsofPlotTabularWindow::onUpdate()
{
  ifstream f(file_.c_str());

  vector< vector<double> > fd;

  string line;
  while (getline(f, line))
  {
    algorithm::trim_left(line);
    char fc; istringstream(line) >> fc; // get first char
    if ( (line.size()==0) || (fc=='#') )
    {
      // comment
    }
    else
    {
      erase_all ( line, "(" );
      erase_all ( line, ")" );
      replace_all ( line, ",", " " );
      replace_all ( line, "\t", " " );
      while (line.find("  ")!=std::string::npos)
      {
        replace_all ( line, "  ", " " );
      }

      vector<string> strs;
      boost::split(strs, line, is_any_of(" "));

      for (const auto& s: strs) std::cout<<s<<" >> "; std::cout<<std::endl;

      vector<double> vals;
      transform(strs.begin(), strs.end(), std::back_inserter(vals),
                [](const std::string& s) { return lexical_cast<double>(s); });

      fd.push_back(vals);
    }
  }

  if (fd.size()==0)
  {
    data_=arma::mat();
  }
  else
  {
    data_.reshape(fd.size(), fd[0].size());
    size_t ir=0;
    for (const auto& r: fd)
    {
      if (r.size()!=data_.n_cols)
        throw insight::Exception(str(format("Wrong number of cols (%d) in data row %d. Expected %d.")%r.size()%ir%data_.n_cols));
      else
      {
        for (size_t j=0;j<r.size(); j++)
          data_(ir,j)=r[j];
      }
      ir++;
    }

    int n_cols=fd[0].size()-1; // first col is time

    // remove unnecessary tabs
    for (int j=ui->graphs->count()-1; j>=n_cols; j--)
    {
      ui->graphs->removeTab(j);
    }
    // add new tabs, if required
    for (int j=ui->graphs->count(); j<n_cols; j++)
    {
      ui->graphs->addTab(new PlotWidget(this),
                         QString::fromStdString(str(format("Col %d")%j))
                         );
    }

    for (int j=1; j<fd[0].size(); j++)
    {
      PlotWidget *p = dynamic_cast<PlotWidget*>(ui->graphs->widget(j-1));
      p->setData(data_.col(0), data_.col(j));
    }
  }


}
