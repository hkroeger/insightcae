#include "qimage.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFile>
#include <QDebug>

namespace insight {



defineType(QImage);
addToFactoryTable(IQResultElement, QImage);


QImage::QImage(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element )
    : IQResultElement(parent, hdmodel, element)
{
  if (auto im = dynamic_cast<insight::Image*>(element)) //could also be a Chart
  {
    QByteArray data;
    {
      // read from stream, because intermediate file will remain locked in windows
      // and disable removal of temporary case directories
      auto f=im->stream();
      char Buffer[128];
      while (!f->eof())
      {
          int BytesIn= f->read(Buffer, sizeof(Buffer)).gcount();
          data.append(Buffer, BytesIn);
      }
      insight::dbg()<<"closing image file"<<std::endl;
    }

    auto pm=std::make_unique<QPixmap>();
    pm->loadFromData(data);
    setImage(std::move(pm));
  }
}

void QImage::setImage(std::unique_ptr<QPixmap> pm)
{
    pm_=std::move(pm);
}

QVariant QImage::previewInformation(int role) const
{
  if (role==Qt::DecorationRole)
  {
    QFontMetrics fm(QApplication::font());
    int h = fm.height();
    auto th = pm_->scaledToHeight(3*h);
    return QVariant(th);
  }

  return QVariant();
}


void QImage::createFullDisplay(QVBoxLayout *layout)
{
  IQResultElement::createFullDisplay(layout);

  auto id=new IQPixmapLabel(*pm_);
  id->setFrameStyle(QFrame::NoFrame);

  layout->addWidget(id);
}


} // namespace insight
