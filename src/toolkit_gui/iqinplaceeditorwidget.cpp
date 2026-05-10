#include "iqinplaceeditorwidget.h"


#include <QApplication>
#include <QKeyEvent>

IQInPlaceEditorWidget::IQInPlaceEditorWidget(
    QSize fixedSize, QWidget *parent)
    : QFrame(parent)
{
    setWindowFlags(Qt::SubWindow);

    setStyleSheet(R"(
        OverlayWidget {
            background-color: #ffffff;
            border: 1px solid #aaaaaa;
            border-radius: 6px;
        }
        QLineEdit {
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 13px;
        }
        QLineEdit:focus {
            border-color: #4a90d9;
        }
        QLabel {
            font-size: 11px;
            color: #555555;
            font-weight: bold;
        }
    )");

    setFixedSize(fixedSize.width(), fixedSize.height());

    // Enter / Esc abfangen
    // m_input1->installEventFilter(this);
    // m_input2->installEventFilter(this);

}

void IQInPlaceEditorWidget::showAt(const QPoint &pos)
{
    // Position so korrigieren, dass das Widget nicht aus dem Eltern-Widget ragt
    QWidget *p = parentWidget();
    int x = pos.x();
    int y = pos.y();
    if (p) {
        x = qMin(x, p->width()  - width()  - 4);
        y = qMin(y, p->height() - height() - 4);
    }
    move(x, y);
    show();
    raise();

    // Fokusverlust überwachen
    connect(qApp, &QApplication::focusChanged,
            this, &IQInPlaceEditorWidget::onFocusChanged);
}

void IQInPlaceEditorWidget::closeOverlay()
{
    hide();
    deleteLater();
}


bool IQInPlaceEditorWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key())
        {
        case Qt::Key_Escape:
            closeOverlay();
            return true;   // Ereignis konsumiert
        default:
            break;
        }
    }
    return QFrame::eventFilter(obj, event);
}



void IQInPlaceEditorWidget::onFocusChanged(QWidget *old, QWidget *now)
{
    if (!isVisible())
        return;

    auto isMine = [this](QWidget* fw) {
        return fw && (fw == this || this->isAncestorOf(fw));
    };

    if (isMine(old) && !isMine(now))
    {
        closeOverlay();
    }
}
