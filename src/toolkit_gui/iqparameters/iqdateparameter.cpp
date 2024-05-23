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
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
        )
    : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{
}


QString IQDateParameter::valueText() const
{
    return QString::fromStdString(
        boost::gregorian::to_simple_string(
        dynamic_cast<const insight::DateParameter&>(parameter())()
            ) );
}


QVBoxLayout* IQDateParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{
    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);
    auto &dp = dynamic_cast<const insight::DateParameter&>(parameter());

    QHBoxLayout *layout2=new QHBoxLayout;
    QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
    layout2->addWidget(promptLabel);
    auto ds=QString::fromStdString(boost::gregorian::to_iso_extended_string(dp()));

    auto*de = new QCalendarWidget(editControlsContainer);
    de->setDateEditEnabled(true);
    de->setSelectedDate(QDate::fromString(ds, Qt::ISODate));
    layout2->addWidget(de);

    layout->addLayout(layout2);

    QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
    layout->addWidget(apply);


    auto applyFunction = [=]()
    {
        auto &dp = dynamic_cast<insight::DateParameter&>(this->parameterRef());
        dp.set(boost::gregorian::from_simple_string(
            de->selectedDate().toString(Qt::ISODate).toStdString()
            ));
    };

    // connect(de, &QLineEdit::returnPressed, applyFunction);
    connect(apply, &QPushButton::pressed, applyFunction);

    // handle external value change
    auto &p = dynamic_cast<insight::DateParameter&>(this->parameterRef());
    ::disconnectAtEOL(
        layout,
        p.valueChanged.connect(
            [=]()
            {
                auto &p = dynamic_cast<const insight::DateParameter&>(parameter());
                QSignalBlocker sb(de);
                de->setSelectedDate(
                    QDate::fromString(
                        QString::fromStdString(boost::gregorian::to_iso_extended_string(p())),
                        Qt::ISODate
                    ) );
            }
            )
        );

    return layout;
}
