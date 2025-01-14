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
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet
        )
    : IQSpecializedParameter<insight::DateTimeParameter>(
          parent, psmodel, parameter, defaultParameterSet )
{
}


QString IQDateTimeParameter::valueText() const
{
    return QString::fromStdString(
        boost::posix_time::to_simple_string(
            parameter()()
            ) );
}


QVBoxLayout* IQDateTimeParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{
    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

    QHBoxLayout *layout2=new QHBoxLayout;
    QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
    layout2->addWidget(promptLabel);

    auto*de = new QDateTimeEdit(editControlsContainer);
    de->setCalendarPopup(true);
    de->setDateTime(QDateTime::fromString(
        QString::fromStdString(
            boost::posix_time::to_iso_extended_string(
                parameter()())),
        Qt::ISODate
        ) );
    layout2->addWidget(de);

    layout->addLayout(layout2);

    QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
    layout->addWidget(apply);


    auto applyFunction = [=]()
    {
        parameterRef().set(boost::posix_time::from_iso_extended_string(
            de->dateTime().toString(Qt::ISODate).toStdString()
            ));
    };

    // connect(de, &QLineEdit::returnPressed, applyFunction);
    connect(apply, &QPushButton::pressed, applyFunction);

    // handle external value change
    ::disconnectAtEOL(
        layout,
        parameterRef().valueChanged.connect(
            [=]()
            {
                QSignalBlocker sb(de);
                de->setDateTime(
                    QDateTime::fromString(
                        QString::fromStdString(
                            boost::posix_time::to_iso_extended_string(
                                parameter()()) ),
                        Qt::ISODate
                        ) );
            }
            )
        );

    return layout;
}
