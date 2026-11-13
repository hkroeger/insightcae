#ifndef IQISCADCHATWIDGET_H
#define IQISCADCHATWIDGET_H

#include <QWidget>
#include <QHash>
#include <QTextDocumentFragment>

#include "assistant/iscadassistantagent.h"

class QTextBrowser;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QTimer;


/**
 * chat with the modeling assistant, which edits the model script
 */
class IQISCADChatWidget
    : public QWidget
{
    Q_OBJECT

    struct Entry
    {
        enum Kind { User, Assistant, Thinking, Tool, Error, Info, Image, Warning, Prompt };
        Kind kind;
        QString text;
    };

    insight::assistant::ISCADAssistantAgent* agent_;
    QList<Entry> entries_;

    /**
     * show the system prompt in the transcript (setting)
     */
    bool showSystemPrompt_ = false;

    /**
     * formatted system prompts by their text (formatting is expensive, render() runs often)
     */
    QHash<QString, QTextDocumentFragment> promptFragments_;

    /**
     * the system prompt and the tools, as shown in the transcript
     */
    QString promptDisplayText() const;

    class Transcript;
    Transcript* transcript_;
    int imageCount_ = 0;
    QPlainTextEdit* input_;
    QPushButton *sendBtn_, *stopBtn_, *newBtn_, *settingsBtn_;
    QLabel *statusLabel_, *costLabel_;

    insight::assistant::Usage lastSeenUsage_, requestUsage_, conversationUsage_;
    double requestCost_ = 0, conversationCost_ = 0;
    QTimer* renderTimer_;

    /**
     * append streamed text to the last entry of this kind or start a new one
     */
    void appendStreamed(Entry::Kind kind, const QString& text);
    void addEntry(Entry::Kind kind, const QString& text);
    void scheduleRender();
    void render();
    void updateButtons();
    void updateUsage(const insight::assistant::Usage& sessionUsage);

    /**
     * create the backend from the stored settings
     */
    void configureBackend();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    IQISCADChatWidget(insight::assistant::ISCADAssistantAgent* agent, QWidget* parent=nullptr);

public Q_SLOTS:
    void send();
    void showSettings();
    void newConversation();
};

#endif // IQISCADCHATWIDGET_H
