
#include <QApplication>
#include <QDebug>
#include <QGraphicsEffect>

#include "qactionprogressdisplayerwidget.h"
#include "qtextensions.h"

#include "boost/algorithm/string.hpp"
#include "base/exception.h"
#include "base/linearalgebra.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <iostream>
#include <cmath>

using namespace std;
using namespace boost;

namespace insight {


void IQProgressWidget::setupUi(const QString& title)
{
    // ── Outer container (gives us drop shadow + rounded corners) ──────────────
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 80));

    card = new QFrame(this);
    card->setObjectName("card");
    card->setGraphicsEffect(shadow);
    card->setStyleSheet(R"(
        QFrame#card {
            background: #ededed;
            border: 1px solid #000000;
            border-radius: 2px;
        }
        QLabel#title {
            #color: #cdd6f4;
            font-weight: 600;
            font-size: 12px;
        }
        QLabel#message {
            #color: #a6adc8;
            font-size: 11px;
        }
        QProgressBar {
            border: none;
            border-radius: 3px;
            background: #313244;
            height: 6px;
            text-align: center;
            color: transparent;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #89b4fa, stop:1 #cba6f7);
            border-radius: 3px;
        }
        QPushButton#cancel {
            background: transparent;
            border: 1px solid #45475a;
            border-radius: 5px;
            color: #f38ba8;
            font-size: 11px;
            padding: 3px 10px;
        }
        QPushButton#cancel:hover {
            background: #f38ba8;
            color: #1e1e2e;
            border-color: #f38ba8;
        }
    )");

    // ── Inner layout ──────────────────────────────────────────────────────────
    titleLabel_   = new QLabel(title, card);
    titleLabel_->setObjectName("title");

    messageLabel_ = new QLabel(tr("Starting…"), card);
    messageLabel_->setObjectName("message");
    messageLabel_->setWordWrap(true);

    progressBar_  = new QProgressBar(card);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setFixedHeight(6);
    progressBar_->setTextVisible(false);

    // Header row: title + cancel button
    auto* headerRow = new QHBoxLayout;
    headerRow->addWidget(titleLabel_, 1);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 10, 14, 12);
    cardLayout->setSpacing(6);
    cardLayout->addLayout(headerRow);
    cardLayout->addWidget(messageLabel_);
    cardLayout->addWidget(progressBar_);

    // Outer layout (transparent, just holds the card so shadow is visible)
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(10, 10, 10, 10);
    outerLayout->addWidget(card);

    setFixedWidth(300);
}





void IQProgressWidget::setCancelButton(bool enabled)
{
    if (enabled)
    {
        if (!cancelButton_)
        {
            insight::dbg()<<"enabling cancel button for action "<<titleLabel_->text().toStdString()<<std::endl;
            auto *headerRow=findContainingLayout(titleLabel_);

            cancelButton_ = new QPushButton(tr("Cancel"), card);
            cancelButton_->setObjectName("cancel");
            cancelButton_->setCursor(Qt::PointingHandCursor);
            connect(cancelButton_, &QPushButton::clicked,
                    this, &IQProgressWidget::onCancelClicked );
            headerRow->addWidget(cancelButton_);
        }
    }
    else
    {
        if (cancelButton_)
        {
            insight::dbg()<<"disabling cancel button for action "<<titleLabel_->text().toStdString()<<std::endl;
            cancelButton_->deleteLater();
            cancelButton_=nullptr;
        }
    }
}





void IQProgressWidget::recheckCancelButton()
{
    // for very short tasks, findAction might fail, because the task is already deleted
    // In this case, just ignore, the widget will be deleted by a subsequently queued finishActionProgress event
    if ( auto *act = trackedAction_.first
          ->findAction(trackedAction_.second) )
    {
        insight::dbg()<<"found action "<<trackedAction_.second<<std::endl;
        if (act->isStoppable())
        {
            insight::dbg()<<"action is stoppable"<<std::endl;
            setCancelButton(true);
        }
        else
        {
            insight::dbg()<<"action is not stoppable"<<std::endl;
            setCancelButton(false);
        }
    }
    else
    {
        insight::dbg()<<"*NOT* found action "<<trackedAction_.second<<std::endl;
        setCancelButton(false);
    }
}


IQProgressWidget::IQProgressWidget(
    const QString& title,
    const Action& action,
    QWidget* parent )
    : QWidget(parent),
    trackedAction_(action)
{
    setAttribute(Qt::WA_TranslucentBackground);

    setupUi(title);
    recheckCancelButton();
}

IQProgressWidget::~IQProgressWidget()
{
    Q_EMIT widgetClosing(this);
}


void IQProgressWidget::onCancelClicked()
{
    if (cancelButton_)
    {
        cancelButton_->setEnabled(false);
        cancelButton_->setText(tr("Cancelling…"));

        trackedAction_.first->triggerStop(trackedAction_.second);
    }
    // The task will emit taskCancelled → onTaskDone → we close.
}


void IQProgressWidget::onStatusMessage(const QString& message)
{
    recheckCancelButton();

    messageLabel_->setText(message);
}

void IQProgressWidget::onProgressChanged(int percent)
{
    recheckCancelButton();

    if (percent < 0)
    {
        // Indeterminate mode
        progressBar_->setRange(0, 0);
    }
    else
    {
        progressBar_->setRange(0, 100);
        progressBar_->setValue(qBound(0, percent, 100));
    }
}









void IQActionProgressDisplayManager::relayout()
{
    if (overlays_.size()<1)
        return;

    const QRect windowRect = hostWidget_->rect();

    // The bottom anchor sits just above the status bar (if any) and the margin.
    const int statusBarH = (statusBar_ && statusBar_->isVisible())
                               ? statusBar_->height()
                               : 0;

    // Start from the bottom and stack upward.
    int bottomY = windowRect.height() - statusBarH - k_margin;

    for (auto o: boost::adaptors::index(overlays_))
    {
        auto* w = o.value().second;

        // Make sure the widget has been properly laid out so sizeHint() is valid.
        w->adjustSize();

        const int ww = w->geometry().width();
        const int wh = w->geometry().height();

        const int x = windowRect.width() - ww - k_margin;
        bottomY -= wh+k_spacing;

        w->setGeometry(x, bottomY, ww, wh);
        w->raise();

    }
}



IQProgressWidget* IQActionProgressDisplayManager::getItem(
    const std::string& path, bool createIfNonExistent )
{
    insight::CurrentExceptionContext ex(
        "returning progress widget for action %s", path.c_str() );

    auto i=overlays_.find(path);
    if (i!=overlays_.end())
    {
        return i->second;
    }
    else
    {
        if (createIfNonExistent)
        {
            insight::CurrentExceptionContext ex2(
                "creating new progress widget for action %s", path.c_str() );

            auto *iqpw=new IQProgressWidget(
                QString::fromStdString(path),
                {this,path},
                hostWidget_ );
            connect(iqpw, &IQProgressWidget::widgetClosing,
                    this, &IQActionProgressDisplayManager::onWidgetClosing );
            overlays_[path]=iqpw;

            iqpw->show();
            QMetaObject::invokeMethod(
                this, &IQActionProgressDisplayManager::relayout,
                Qt::QueuedConnection);

            return iqpw;
        }
    }

    return nullptr;
}

bool IQActionProgressDisplayManager::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == hostWidget_)
    {
        if (event->type() == QEvent::Resize)
            relayout();
    }
    else if (watched == statusBar_)
    {
        switch (event->type())
        {
        case QEvent::Show:
        case QEvent::Hide:
        case QEvent::Resize:
            relayout();
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(watched, event);
}



void IQActionProgressDisplayManager::onWidgetClosing(IQProgressWidget *widget)
{
    auto i=std::find_if(
        overlays_.begin(), overlays_.end(),
        [widget](const decltype(overlays_)::value_type& i)
        { return i.second==widget; } );
    if (i!=overlays_.end())
    {
        overlays_.erase(i);
        // widget will be deleted via deleteLater() by itself; just relayout the rest.
        relayout();
    }
}


IQActionProgressDisplayManager::IQActionProgressDisplayManager(
    QWidget *parent, QWidget* statusBar )
  : QObject(parent),
    hostWidget_(parent),
    statusBar_(statusBar)
{
    parent->installEventFilter(this);
    if (statusBar_)
        statusBar_->installEventFilter(this);

}




void IQActionProgressDisplayManager::update(const ProgressState &/*pi*/)
{}




void IQActionProgressDisplayManager::logMessage(const std::string &line)
{}




void IQActionProgressDisplayManager::setActionProgressValue(
    const std::string &path, double value )
{
  QMetaObject::invokeMethod( // post into GUI thread as this method might be called from different thread
        this,
        [this,path,value]()
        {
            if (auto i=getItem(
                    path,
                    fabs(value)<SMALL // allow progress widget creation only on first call with value=0
                    ))
            {
                i->onProgressChanged(std::round(100.*value));
            }
        },
        Qt::QueuedConnection
  );
}




void IQActionProgressDisplayManager::setMessageText(
    const std::string &path, const std::string &message )
{
  QMetaObject::invokeMethod( // post into GUI thread as this method might be called from different thread
        this,
        [this,path,message]()
        {
            try
            {
                if (auto i=getItem(path, false))
                {
                    insight::dbg()<<"setting text for "<<path<<" to "<<message<<std::endl;
                    i->onStatusMessage( QString::fromStdString(message) );
                }
            }
            catch (const insight::Exception& ex)
            {
                // ignore, progress bar was either not yet created or already removed
            }
        },
        Qt::QueuedConnection
  );
}




void IQActionProgressDisplayManager::finishActionProgress(const string &path)
{
  QMetaObject::invokeMethod( // post into GUI thread as this method might be called from different thread
        this,
        [this,path]()
        {
            insight::CurrentExceptionContext ex("removing progress widget %s", path.c_str());
            if (auto i=getItem(path, false))
            {
                i->deleteLater();
                overlays_.erase(path);
            }
        },
        Qt::QueuedConnection
  );
}




void IQActionProgressDisplayManager::reset()
{
    for (auto& c: overlays_)
    {
        c.second->deleteLater();
    }
    overlays_.clear();
}




} // namespace insight
