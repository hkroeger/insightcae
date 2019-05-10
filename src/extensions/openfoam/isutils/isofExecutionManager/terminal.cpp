#include "terminal.h"

// QtCore
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QObject>
#include <QString>
#include <QUrl>
// QtGui
#include <QKeyEvent>
#include <QClipboard>
// QtWidgets
#include <QHBoxLayout>
#include <QApplication>
#include <QWidget>

#include <kde_terminal_interface.h>
#include <KCoreAddons/KPluginLoader>
#include <KCoreAddons/KPluginFactory>
#include <KI18n/KLocalizedString>
#include <KService/KService>
#include <KWidgetsAddons/KToggleAction>
#include <KWidgetsAddons/KMessageBox>

#include <signal.h>


TerminalWidget::TerminalWidget(QWidget *parent)
  : QWidget(parent),
    konsole_part(nullptr), t(nullptr), initialised(false), firstInput(true)
{
  terminal_hbox = new QHBoxLayout(this);
}

bool TerminalWidget::initialise()
{
  if (! initialised) { // konsole part is not yet loaded or it has already failed
      KService::Ptr service = KService::serviceByDesktopName("konsolepart");

      if (service) {
          QWidget *focusW = qApp->focusWidget();
          // Create the part
          QString error;
          konsole_part = service->createInstance<KParts::ReadOnlyPart>(this, this, QVariantList(), &error);

          if (konsole_part) { //loaded successfully
              terminal_hbox->addWidget(konsole_part->widget());
              setFocusProxy(konsole_part->widget());
//              connect(konsole_part, &KParts::ReadOnlyPart::destroyed, this,&TerminalDock::killTerminalEmulator);

              // must filter app events, because some of them are processed
              // by child widgets of konsole_part->widget()
              // and would not be received on konsole_part->widget()
              qApp->installEventFilter(this);
              t = qobject_cast<TerminalInterface*>(konsole_part);
//              if (t) {
//                  lastPath = QDir::currentPath();
//                  t->showShellInDir(lastPath);
//              }
              initialised = true;
              firstInput = true;
          } else
              KMessageBox::error(nullptr, i18n("<b>Cannot create embedded terminal.</b><br/>"
                                         "The reported error was: %1", error));
          // the Terminal Emulator may be hidden (if we are creating it only
          // to send command there and see the results later)
          if (focusW) {
              focusW->setFocus();
          }/* else {
              ACTIVE_PANEL->gui->slotFocusOnMe();
          }*/
      } else
          KMessageBox::sorry(nullptr, i18nc("missing program - arg1 is a URL",
                                      "<b>Cannot create embedded terminal.</b><br>"
                                      "You can fix this by installing Konsole:<br/>%1",
                                      QString("<a href='%1'>%1</a>").arg(
                                          "https://www.kde.org/applications/system/konsole")),
                             nullptr, KMessageBox::AllowLink);
  }
  return isInitialised();

}

bool TerminalWidget::isInitialised() const
{
    return konsole_part != nullptr && konsole_part->widget() != nullptr;
}


void TerminalWidget::sendInput(const QString& input, bool clearCommand)
{
    if (!t)
        return;

    if (clearCommand)
    {
        // send SIGINT before input command to avoid unwanted behaviour when current line is not empty
        // and command is appended to current input (e.g. "rm -rf x " concatenated with 'cd /usr');
        // code "borrowed" from Dolphin, Copyright (C) 2007-2010 by Peter Penz <peter.penz19@gmail.com>
        const int processId = t->terminalProcessId();
        // workaround (firstInput): kill is sent to terminal if shell is not initialized yet
        if (processId > 0 && !firstInput)
        {
            kill(processId, SIGINT);
        }
    }
    firstInput = false;

    t->sendInput(input);
}

void TerminalWidget::setDirectory(const boost::filesystem::path& path)
{
    // A space exists in front of the `cd` so as to avoid that Krusader's embedded terminal adds a lot of `cd`
    // messages to the shell history, in Dolphin it's done the same way: https://bugs.kde.org/show_bug.cgi?id=204039
    sendInput(QString(" cd \"") + path.c_str() + QString("\"\n"));
}
