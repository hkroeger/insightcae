#ifndef TERMINAL_H
#define TERMINAL_H

#ifdef HAVE_KF5


#include <QEvent>
#include <QString>
#include <QObject>

#include <QHBoxLayout>
#include <QWidget>

#include <kde_terminal_interface.h>
#include <KParts/ReadOnlyPart>

#include "base/boost_include.h"

class TerminalWidget
        : public QWidget
{
    QHBoxLayout *terminal_hbox;             // hbox for terminal_dock
    KParts::ReadOnlyPart* konsole_part;     // the actual part pointer
    TerminalInterface* t;                   // TerminalInterface of the konsole part
    bool initialised;
    bool firstInput;

public:
  TerminalWidget(QWidget *parent=nullptr);

  bool initialise();

  bool isInitialised() const;

  void sendInput(const QString& input, bool clearCommand=true);
  void setDirectory(const boost::filesystem::path& path);
};

#endif

#endif // TERMINAL_H
