#include "logviewerwidget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTemporaryFile>
#include <QScrollBar>

#include "email.h"
#include "base/progressdisplayer.h"

LogViewerWidget::LogViewerWidget(QWidget* parent)
  : QPlainTextEdit(parent)
{
  setMaximumBlockCount(10000);
  QTextDocument *doc = document();
  doc->setDefaultStyleSheet("p, b, font, li { white-space: pre-wrap; }");
  QFont font("unexistent");
  font.setStyleHint(QFont::Monospace);
  doc->setDefaultFont(font);
}


void LogViewerWidget::appendLine(const QString& line)
{
  appendPlainText(line);
}

void LogViewerWidget::appendErrorLine(const QString &line)
{
  appendHtml("<b>"+line+"</b>");
}

void LogViewerWidget::appendDimmedLine(const QString &line)
{
  appendHtml("<font color=\"Gray\">"+line+"</font>");
}

void LogViewerWidget::appendLogMessage(const insight::ProgressState &ps)
{
  appendPlainText( QString::fromStdString(ps.logMessage_) );
}



void LogViewerWidget::saveLog()
{
    QString fn=QFileDialog::getSaveFileName(
        this,
        "Save Log to file",
        "",
        "Log file (*.txt)"
    );

    if (!fn.isEmpty())
    {
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::critical(this, "Create file failed", "Could not create file "+fn);
            return;
        }
        QTextStream out(&f);
        out << toPlainText();
    }
}

void LogViewerWidget::sendLog()
{
    QTemporaryFile f;
    if (!f.open())
    {
        QMessageBox::critical(this, "Creation of temporary file failed",
                              "Could not create temporary file "+f.fileName());
        return;
    }
    QTextStream out(&f);
    out<< toPlainText();
    out.flush();

    Email e;
    e.setReceiverAddress("info@silentdynamics.de");
    e.setSubject("Analysis Log");
    e.addAttachment(QFileInfo(f).canonicalFilePath());
    e.openInDefaultProgram();
}


void LogViewerWidget::clearLog()
{
  clear();
}

void LogViewerWidget::autoScrollLog()
{
  verticalScrollBar()->setValue( verticalScrollBar()->maximum() );
}
