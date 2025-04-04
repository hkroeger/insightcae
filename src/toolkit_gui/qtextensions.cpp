#include "qtextensions.h"
#include "base/exception.h"
#include "base/translations.h"

#include <QFileDialog>
#include <QDebug>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string.hpp>
#include <qnamespace.h>




void disconnectAtEOL(QObject *o, const boost::signals2::connection &connection)
{
    QObject::connect(
        o, &QObject::destroyed, o,
            [connection]()
            {
              connection.disconnect();
            }
        );
}




FileTypeByExtension::FileTypeByExtension(std::string ext, std::string desc, bool isDefl)
    : extension(ext), description(desc), isDefault(isDefl)
{}


bool FileTypeByExtension::operator<(const FileTypeByExtension &o) const
{
    return extension < o.extension;
}




getFileName::getFileName(
    QWidget* parent,
    const QString &caption,
    GetFileMode mode,
    const std::set<FileTypeByExtension>& extensions,
    const boost::optional<boost::filesystem::path>& startDir,
    std::function<void(QGridLayout *)> setupAdditionalControls
    )
{
    std::map<QString,std::string> extKeys;

    QStringList filters;
    QString selFilter;

    for (auto& e: extensions)
    {
        auto ext=boost::to_lower_copy(e.extension);
        std::vector<std::string> variants;
        boost::split(
            variants, ext,
            boost::is_any_of(" \t,;"),
            boost::token_compress_on);
        insight::assertion(
            variants.size(),
            "internal error: no extensions given for file type");

        std::vector<std::string> pats;
        std::transform(
            variants.begin(), variants.end(),
            std::back_inserter(pats),
            [](const std::string& e) { return "*."+e; });
        auto ee=QString::fromStdString(
            e.description+" ("+boost::join(pats, " ")+")"
        );

        if (ext.find('*')==std::string::npos)
        {
            extKeys[ee]=variants.front();
        }
        filters+=ee;
        if (e.isDefault) selFilter=ee;
    }


    if (selFilter.isEmpty() && extKeys.size()) // no default specified
    {
        selFilter=extKeys.begin()->first;
    }

    QFileDialog fd(parent);
    fd.setOption(QFileDialog::DontUseNativeDialog, true);
    fd.setNameFilters(filters);
    fd.selectNameFilter(selFilter);
    if (!caption.isEmpty())
    {
        fd.setWindowTitle(caption);
    }
    if (startDir)
    {
        fd.setDirectory(
            QString::fromStdString(startDir->string()));
    }

    QString fn;
    if (mode == GetFileMode::Save)
    {
        fd.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
        if (caption.isEmpty())
        {
            fd.setWindowTitle(_("Save file"));
        }
    }
    else if (mode == GetFileMode::Open)
    {
        fd.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
        if (caption.isEmpty())
        {
            fd.setWindowTitle(_("Open file"));
        }
    }
    else
        throw insight::UnhandledSelection();

    if (setupAdditionalControls)
    {
        setupAdditionalControls(
            static_cast<QGridLayout*>(fd.layout()));
    }

    if (fd.exec() == QDialog::Accepted)
    {
        if (fd.selectedFiles().size())
        {
            QString fn = fd.selectedFiles()[0];
            boost::filesystem::path res(fn.toStdString());
            if (res.extension().empty())
            {
                auto iex=extKeys.find(
                    fd.selectedNameFilter());

                if (iex!=extKeys.end())
                {
                    res+="."+iex->second;
                }
            }
            selectedPath_=res;
        }
    }
}

const boost::filesystem::path &getFileName::asFilesystemPath() const
{
    insight::assertion(
        bool(selectedPath_),
        "tried to query filename but none was selected"
        );
    return *selectedPath_;
}


std::string getFileName::asString() const
{
    return asFilesystemPath().string();
}

QString getFileName::asQString() const
{
    return QString::fromStdString(asString());
}


getFileName::operator bool() const
{
    return bool(selectedPath_);
}

getFileName::operator QString() const
{
    return asQString();
}

getFileName::operator boost::filesystem::path() const
{
    return asFilesystemPath();
}



IQEphemeralLabel::IQEphemeralLabel(QWidget *parent)
    : QLabel(parent)
{}



void IQEphemeralLabel::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);
    hide();
}


IQPixmapLabel::IQPixmapLabel(const QPixmap& pm, QWidget *parent) :
    QLabel(parent),
    pixmap_(pm)
{
    this->setMinimumSize(1,1);
    setScaledContents(false);
    QSizePolicy qsp(QSizePolicy::Preferred,QSizePolicy::Preferred);
    qsp.setHeightForWidth(true);
    setSizePolicy(qsp);
    QLabel::setPixmap(scaledPixmap());
}


int IQPixmapLabel::heightForWidth( int width ) const
{
    return pixmap_.isNull() ?
               this->height()
            : double(pixmap_.height()*width)/double(pixmap_.width());
}

QSize IQPixmapLabel::sizeHint() const
{
    int w = this->width();
    return QSize( w, heightForWidth(w) );
}

QPixmap IQPixmapLabel::scaledPixmap() const
{
    return pixmap_.scaledToWidth(
        this->size().width(),
        Qt::SmoothTransformation);
}

const QPixmap &IQPixmapLabel::originalPixmap() const
{
    return pixmap_;
}

void IQPixmapLabel::resizeEvent(QResizeEvent * e)
{
    if(!pixmap_.isNull())
        QLabel::setPixmap(scaledPixmap());
}


void IQSimpleLatexView::updateContent()
{
    auto w = viewport()->width();
    if (w!=cur_content_width_)
    {
        cur_content_width_=w;
        setHtml(
            QString::fromStdString(
                content_.toHTML(cur_content_width_) ) );
    }
}


IQSimpleLatexView::IQSimpleLatexView(
    const insight::SimpleLatex &slt, QWidget *parent)
: QTextEdit(parent),
    content_(slt),
    cur_content_width_(-1)
{
    setReadOnly(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setFrameStyle(QFrame::NoFrame|QFrame::Plain);
    viewport()->setAutoFillBackground(false); // BG like QLabel

    setMinimumHeight(2*QFontMetrics(font()).lineSpacing());
    QSizePolicy qsp(
        QSizePolicy::Preferred,
        QSizePolicy::MinimumExpanding);
    qsp.setHeightForWidth(true);
    setSizePolicy(qsp);

    updateContent();
}


int IQSimpleLatexView::heightForWidth(int width) const
{
    QTextDocument td;
    td.setHtml(document()->toHtml());
    td.setTextWidth(width);
    td.adjustSize();
    auto h = td.size().height();
    return h;
}

QSize IQSimpleLatexView::sizeHint() const
{
    QTextDocument td;
    td.setHtml(document()->toHtml());
    if (cur_content_width_>0)
        td.setTextWidth(cur_content_width_);
    else
        td.setTextWidth(td.idealWidth());
    td.adjustSize();
    auto ts=td.size();
    return QSize(ts.width(), ts.height());
}

void IQSimpleLatexView::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    updateContent();
}
