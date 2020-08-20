#include "qimage.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>

namespace insight {


defineType(QImage);
addToFactoryTable(QResultElement, QImage);


QImage::QImage(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : QResultElement(parent, label, rep)
{
  if (auto im = resultElementAs<insight::Image>())
  {
    setImage( QPixmap(QString::fromStdString(im->imagePath().string())) );
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
//  id_->setFrameShape(QFrame::NoFrame);
  id_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
//  id_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
//  id_->setScaledContents(true);

  sa_ = new QScrollArea;
//  sa_->setBackgroundRole(QPalette::Dark);
  QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sp.setHeightForWidth(false);
  sp.setWidthForHeight(false);
  sp.setVerticalStretch(255);
  sa_->setSizePolicy(sp);
  sa_->setWidget(id_);

  layout->addWidget(sa_);
}

void QImage::resetContents(int width, int height)
{
  QResultElement::resetContents(width, height);

  id_->setPixmap(image_.scaledToWidth(sa_->contentsRect().width()));
  id_->adjustSize();
}

} // namespace insight
