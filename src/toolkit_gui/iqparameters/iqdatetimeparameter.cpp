#include "iqdatetimeparameter.h"

#include "boost/date_time/gregorian/formatters.hpp"
#include "boost/date_time/gregorian/parsers.hpp"
#include "boost/date_time/posix_time/time_formatters.hpp"
#include "boost/date_time/posix_time/time_parsers.hpp"
#include "qdatetime.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QCalendarWidget>

#include "qnamespace.h"
#include "qtextensions.h"

defineType(IQDateTimeParameter);
addToFactoryTable(IQParameter, IQDateTimeParameter);

IQDateTimeParameter::IQDateTimeParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
        )
    : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{
}


QString IQDateTimeParameter::valueText() const
{
    return QString::fromStdString(
        boost::posix_time::to_simple_string(
            dynamic_cast<const insight::DateTimeParameter&>(parameter())()
            ) );
}


QVBoxLayout* IQDateTimeParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{
    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);
    auto &dp = dynamic_cast<const insight::DateTimeParameter&>(parameter());

    QHBoxLayout *layout2=new QHBoxLayout;
    QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
    layout2->addWidget(promptLabel);

    auto*de = new QDateTimeEdit(editControlsContainer);
    de->setCalendarPopup(true);
    de->setDateTime(QDateTime::fromString(
        QString::fromStdString(boost::posix_time::to_iso_extended_string(dp())),
        Qt::ISODate
        ) );
    layout2->addWidget(de);

    layout->addLayout(layout2);

    QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
    layout->addWidget(apply);


    auto applyFunction = [=]()
    {
        auto &dp = dynamic_cast<insight::DateTimeParameter&>(this->parameterRef());
        dp.set(boost::posix_time::from_iso_extended_string(
            de->dateTime().toString(Qt::ISODate).toStdString()
            ));
    };

    // connect(de, &QLineEdit::returnPressed, applyFunction);
    connect(apply, &QPushButton::pressed, applyFunction);

    // handle external value change
    auto &p = dynamic_cast<insight::DateTimeParameter&>(this->parameterRef());
    ::disconnectAtEOL(
        layout,
        p.valueChanged.connect(
            [=]()
            {
                auto &p = dynamic_cast<const insight::DateTimeParameter&>(parameter());
                QSignalBlocker sb(de);
                de->setDateTime(
                    QDateTime::fromString(
                        QString::fromStdString(boost::posix_time::to_iso_extended_string(p())),
                        Qt::ISODate
                        ) );
            }
            )
        );

    return layout;
}
