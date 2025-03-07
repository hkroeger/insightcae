#ifndef QTEXTENSIONS_H
#define QTEXTENSIONS_H

#include <set>
#include "base/boost_include.h"

#include <QObject>
#include <QGridLayout>

#include "boost/signals2.hpp"



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



#endif // QTEXTENSIONS_H
