#ifndef QTEXTENSIONS_H
#define QTEXTENSIONS_H

#include <set>

#include "base/boost_include.h"
#include "base/latextools.h"

#include <QObject>
#include <QGridLayout>
#include <QTextEdit>
#include <QLabel>
#include <QResizeEvent>

#include "boost/signals2.hpp"

#include "toolkit_gui_export.h"


// helper for boost::signals: disconnect at QObject destruction
void disconnectAtEOL(
    QObject *o,
    const boost::signals2::connection& connection
    );




struct FileTypeByExtension
{
    std::string extension;
    std::string description;
    bool isDefault;

    FileTypeByExtension(
        std::string ext = "*",
        std::string desc = "All files",
        bool isDefl = false
        );

    bool operator<(const FileTypeByExtension& o) const;
};



enum GetFileMode { Open, Save };


class getFileName
{
    boost::optional<boost::filesystem::path> selectedPath_;

public:
    getFileName(
     QWidget* parent,
     const QString &caption,
     GetFileMode mode,
     const std::set<FileTypeByExtension>& extensions
        = { {"*", "all files", true} },
     const boost::optional<boost::filesystem::path>& startDir
        = boost::none,
     std::function<void(QGridLayout *)> setupAdditionalControls
        = std::function<void(QGridLayout *)>()
    );


    const boost::filesystem::path& asFilesystemPath() const;
    std::string asString() const;
    QString asQString() const;

    operator bool() const;
    operator QString() const;
    operator boost::filesystem::path() const;
};



/**
 * @brief The IQEphemeralLabel class
 * disappears on mouseclick
 */
class TOOLKIT_GUI_EXPORT IQEphemeralLabel : public QLabel
{
public:
    IQEphemeralLabel(QWidget *parent=nullptr);

    void mousePressEvent(QMouseEvent *event) override;
};



class TOOLKIT_GUI_EXPORT IQPixmapLabel
    : public QLabel
{
    Q_OBJECT

    QPixmap pixmap_;

public:
    IQPixmapLabel(const QPixmap &pm, QWidget *parent = nullptr);

    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QPixmap scaledPixmap() const;
    const QPixmap& originalPixmap() const;

public Q_SLOTS:
    void resizeEvent(QResizeEvent *) override;

};


class TOOLKIT_GUI_EXPORT IQSimpleLatexView
    : public QTextEdit
{
    Q_OBJECT

    insight::SimpleLatex content_;
    int cur_content_width_;

    void updateContent();

public:
    IQSimpleLatexView(const insight::SimpleLatex &slt, QWidget *parent = nullptr);
    int heightForWidth(int width) const override;

    QSize sizeHint() const override;

public Q_SLOTS:
    void resizeEvent(QResizeEvent *) override;

};

#endif // QTEXTENSIONS_H
