#ifndef OF_CLEAN_CASE_H
#define OF_CLEAN_CASE_H

#include <QWidget>
#include <QDialog>
#include <QTimer>

#include "base/boost_include.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

namespace Ui
{
class OFCleanCaseForm;
}


class OFCleanCaseForm
 : public QWidget
{
    Q_OBJECT

    Ui::OFCleanCaseForm* ui;

    const insight::OpenFOAMCase& ofc_;
    boost::filesystem::path location_;
    std::shared_ptr<insight::OpenFOAMCaseDirs> cf_;

    QTimer *t_;

public:
    OFCleanCaseForm(const insight::OpenFOAMCase& ofc, const boost::filesystem::path& location, QWidget* parent=NULL);

    insight::OpenFOAMCaseDirs::TimeDirOpt timeStepSelection();
private Q_SLOTS:
    void updateCandidateList();
    void executeDeletion();

Q_SIGNALS:
    void statusMessage(const QString& msg);
};

class OFCleanCaseDialog
: public QDialog
{
public:
  OFCleanCaseDialog(const insight::OpenFOAMCase& ofc, const boost::filesystem::path& location, QWidget* parent =nullptr);
};

#endif // OF_CLEAN_CASE_H
