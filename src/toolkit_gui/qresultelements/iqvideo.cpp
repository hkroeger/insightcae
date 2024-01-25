#include "iqvideo.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFile>
#include <QDebug>

#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoWidget>


namespace insight {




defineType(IQVideo);
addToFactoryTable(IQResultElement, IQVideo);



IQVideo::IQVideo(QObject *parent, const QString &label, insight::ResultElementPtr rep)
    : IQResultElement(parent, label, rep),
    delta_w_(0)
{
    if (auto im = resultElementAs<insight::Video>())
    {
        // QByteArray data;
        // {
        //     // read from stream, because intermediate file will remain locked in windows
        //     // and disable removal of temporary case directories
        //     auto& f=im->stream();
        //     char Buffer[128];
        //     while (!f.eof())
        //     {
        //         int BytesIn= f.read(Buffer, sizeof(Buffer)).gcount();
        //         data.append(Buffer, BytesIn);
        //     }
        //     insight::dbg()<<"closing image file"<<std::endl;
        // }
        // QPixmap pm;
        // pm.loadFromData(data);
        // setImage( pm );
    }
}




void IQVideo::setPreviewImage(const QPixmap &pm)
{
    previewImage_=pm;
}




QVariant IQVideo::previewInformation(int role) const
{
    if (role==Qt::DecorationRole)
    {
        // QFontMetrics fm(QApplication::font());
        // int h = fm.height();
        // return QVariant(previewImage_.scaledToHeight(3*h));
    }

    return QVariant();
}




void IQVideo::createFullDisplay(QVBoxLayout *layout)
{
    IQResultElement::createFullDisplay(layout);

    id_=new QLabel;
    id_->setFrameShape(QFrame::NoFrame);
    id_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);


    auto player = new QMediaPlayer(layout);

    auto playlist = new QMediaPlaylist(player);

    auto im = resultElementAs<insight::Video>();
    playlist->addMedia(QUrl::fromLocalFile(im->filePath().string().c_str()));

    auto videoWidget = new QVideoWidget;
    player->setVideoOutput(videoWidget);

    // videoWidget->show();
    playlist->setCurrentIndex(1);
    player->play();

    // sa_ = new QScrollArea;
    // QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // sp.setHeightForWidth(false);
    // sp.setWidthForHeight(false);
    // sp.setVerticalStretch(255);
    // sa_->setSizePolicy(sp);
    // sa_->setWidget(id_);

    layout->addWidget(videoWidget);

    // auto cmp = layout->parentWidget()->contentsMargins();
    // auto cmsa = sa_->contentsMargins();
    // delta_w_ = 2*layout->margin() + cmsa.left() + cmsa.right() + cmp.left() + cmp.right();
}




void IQVideo::resetContents(int width, int height)
{
    IQResultElement::resetContents(width, height);

    // id_->setPixmap(image_.scaledToWidth(width-delta_w_));
    // id_->adjustSize();
}

} // namespace insight

