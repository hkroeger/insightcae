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
addToFactoryTable(QResultElement, QImage);


QImage::QImage(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : QResultElement(parent, label, rep),
      delta_w_(0)
{
  if (auto im = resultElementAs<insight::Image>())
  {
    QByteArray data;
    {
      // read from stream, because intermediate file will remain locked in windows
      // and disable removal of temporary case directories
      auto& f=im->stream();
      char Buffer[128];
      while (!f.eof())
      {
          int BytesIn= f.read(Buffer, sizeof(Buffer)).gcount();
          data.append(Buffer, BytesIn);
      }
      insight::dbg()<<"closing image file"<<std::endl;
    }
    QPixmap pm;
    pm.loadFromData(data);
    setImage( pm );
  }
}

void QImage::setImage(const QPixmap &pm)
{
  image_=pm;
}

QVariant QImage::previewInformation(int role) const
{
  if (role==Qt::DecorationRole)
  {
    QFontMetrics fm(QApplication::font());
    int h = fm.height();
    return QVariant(image_.scaledToHeight(3*h));
  }

  return QVariant();
}


void QImage::createFullDisplay(QVBoxLayout *layout)
{
  QResultElement::createFullDisplay(layout);

  id_=new QLabel;
  id_->setFrameShape(QFrame::NoFrame);
  id_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);


  sa_ = new QScrollArea;
  QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sp.setHeightForWidth(false);
  sp.setWidthForHeight(false);
  sp.setVerticalStretch(255);
  sa_->setSizePolicy(sp);
  sa_->setWidget(id_);

  layout->addWidget(sa_);

  auto cmp = layout->parentWidget()->contentsMargins();
  auto cmsa = sa_->contentsMargins();
  delta_w_ = 2*layout->margin() + cmsa.left() + cmsa.right() + cmp.left() + cmp.right();
}

void QImage::resetContents(int width, int height)
{
  QResultElement::resetContents(width, height);

  id_->setPixmap(image_.scaledToWidth(width-delta_w_));
  id_->adjustSize();
}

} // namespace insight
