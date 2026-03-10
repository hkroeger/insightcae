#ifndef IQINPLACEEDITORWIDGET_H
#define IQINPLACEEDITORWIDGET_H

#include <QFrame>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>

class IQInPlaceEditorWidget
    : public QFrame
{
    Q_OBJECT

public:
    explicit IQInPlaceEditorWidget(QSize fixedSize, QWidget *parent = nullptr);
    void showAt(const QPoint &pos);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeOverlay();

private Q_SLOTS:
    void onFocusChanged(QWidget *old, QWidget *now);
};

#endif // IQINPLACEEDITORWIDGET_H
