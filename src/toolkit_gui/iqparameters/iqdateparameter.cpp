#include "iqdateparameter.h"
#include "boost/date_time/gregorian/formatters.hpp"
#include "boost/date_time/gregorian/parsers.hpp"
#include "qdatetime.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QCalendarWidget>

#include "qnamespace.h"
#include "qtextensions.h"

defineType(IQDateParameter);
addToFactoryTable(IQParameter, IQDateParameter);

IQDateParameter::IQDateParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet
        )
    : IQSpecializedParameter<insight::DateParameter>(
          parent, psmodel, parameter, defaultParameterSet )
{
}


QString IQDateParameter::valueText() const
{
    return QString::fromStdString(
        boost::gregorian::to_simple_string(
            parameter()()
            ) );
}


QVBoxLayout* IQDateParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{
    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

    QHBoxLayout *layout2=new QHBoxLayout;
    QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
    layout2->addWidget(promptLabel);
    auto ds=QString::fromStdString(
        boost::gregorian::to_iso_extended_string(
            parameter()()));

    auto*de = new QCalendarWidget(editControlsContainer);
    de->setDateEditEnabled(true);
    de->setSelectedDate(QDate::fromString(ds, Qt::ISODate));
    layout2->addWidget(de);

    layout->addLayout(layout2);

    QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
    layout->addWidget(apply);


    auto applyFunction = [=]()
    {
        parameterRef().set(boost::gregorian::from_simple_string(
            de->selectedDate().toString(Qt::ISODate).toStdString()
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
                de->setSelectedDate(
                    QDate::fromString(
                        QString::fromStdString(
                            boost::gregorian::to_iso_extended_string(
                                parameter()()) ),
                        Qt::ISODate
                    ) );
            }
            )
        );

    return layout;
}
