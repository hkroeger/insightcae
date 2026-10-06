#ifndef IQTRAFFICLIGHT_H
#define IQTRAFFICLIGHT_H

#include "toolkit_gui_export.h"

#include <QWidget>

/**
 * @brief The IQTrafficLight class
 * displays a status as a small traffic light (red, yellow, green).
 * Vertical (red on top) by default.
 * In state Unknown, all lights are dimmed.
 */
class TOOLKIT_GUI_EXPORT IQTrafficLight
    : public QWidget
{
    Q_OBJECT

public:
    enum State { Unknown, Green, Yellow, Red };

private:
    State state_;
    Qt::Orientation orientation_;

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    explicit IQTrafficLight(QWidget* parent = nullptr, Qt::Orientation o = Qt::Vertical);

    State state() const;
    void setState(State s);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation o);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
};

#endif // IQTRAFFICLIGHT_H
