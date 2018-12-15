#ifndef INSERTEDCASEELEMENT_H
#define INSERTEDCASEELEMENT_H

#include <QListWidget>
#include <QListWidgetItem>

#include <string>

#ifndef Q_MOC_RUN
#include "openfoam/openfoamcase.h"
#endif

class InsertedCaseElement
: public QListWidgetItem
{
    std::string type_name_;
    insight::ParameterSet curp_;

public:
    InsertedCaseElement(QListWidget*, const std::string& type_name);

    inline const std::string& type_name() const { return type_name_; }
    inline insight::ParameterSet& parameters() { return curp_; }
    void insertElement(insight::OpenFOAMCase& ofc) const;
};



#endif // INSERTEDCASEELEMENT_H
