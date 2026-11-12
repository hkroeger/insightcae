#ifndef IQISCADCHATWIDGET_H
#define IQISCADCHATWIDGET_H

#include <QWidget>

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
        enum Kind { User, Assistant, Thinking, Tool, Error, Info };
        Kind kind;
        QString text;
    };

    insight::assistant::ISCADAssistantAgent* agent_;
    QList<Entry> entries_;

    QTextBrowser* transcript_;
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
