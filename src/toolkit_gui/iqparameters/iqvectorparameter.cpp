#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <memory>

#include "iqvectorparameter.h"
#include "base/linearalgebra.h"
#include "iqparametersetmodel.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvectordirectioncommand.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerpickpoint.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkmanipulatecoordinatesystem.h"

defineType(IQVectorParameter);
addToFactoryTable(IQParameter, IQVectorParameter);

IQVectorParameter::IQVectorParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
) : IQSpecializedParameter<insight::VectorParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{}


QString IQVectorParameter::valueText() const
{
  return QString("[%1]")
        .arg(QString::fromStdString(
            insight::valueToString(parameter()())
            ));
}



QVBoxLayout* IQVectorParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  lineEdit=new QLineEdit(editControlsContainer);

  lineEdit->setText( QString::fromStdString(
      insight::valueToString(parameter()()) ));

  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  QPushButton *dlgBtn_=nullptr;
  if (viewer)
  {
      dlgBtn_ = new QPushButton("...", editControlsContainer);
      layout->addWidget(dlgBtn_);
  }

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
      arma::mat v;
      insight::stringToValue(lineEdit->text().toStdString(), v);
      parameterRef().set(v);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

#warning need generalized implementation
  if (auto *v = dynamic_cast<IQVTKCADModel3DViewer*>(viewer))
  {
    connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
          [this,v,apply,applyFunction]()
          {

            if (auto bp =
              model()->getVectorBasePoint(parameter().path()))
            {
                // auto curMod =
                //       new IQVectorDirectionCommand(
                //             v->interactor(),
                //             (*bp), p() );

                // connect( apply, &QPushButton::pressed,
                //          curMod, &QObject::deleteLater );

                // connect( curMod, &IQVectorDirectionCommand::dataChanged, curMod,
                //          [this,curMod]()
                //          {
                //            lineEdit->setText(
                //                        QString::fromStdString(
                //                            insight::valueToString(
                //                                curMod->getVector()
                //                                ) ) );
                //          } );
                auto mani = make_viewWidgetAction<IQVTKManipulateCoordinateSystem>(
                    *v->topmostActionHost(), insight::CoordinateSystem((*bp), parameter()()), true );
                connect(mani.get(), &IQVTKManipulateCoordinateSystem::coordinateSystemSelected,
                        [this,applyFunction](const insight::CoordinateSystem& cs)
                        {
                            parameterRef().set(cs.ex);
                        }
                        );
                v->topmostActionHost()->launchAction(std::move(mani));
            }
            else
            {
              auto ppc = make_viewWidgetAction<IQVTKCADModel3DViewerPickPoint>(
                    *v->topmostActionHost() );
              connect(ppc.get(), &IQVTKCADModel3DViewerPickPoint::pickedPoint,
                        [this,applyFunction](const arma::mat& pt)
                        {
                          parameterRef().set(pt);
                        }
                      );
              v->topmostActionHost()->launchAction(std::move(ppc));
            }
          }
    );
  }

  return layout;
}

void IQVectorParameter::applyProposition(
    const insight::ParameterSet &propositions,
    const std::string &selectedProposition)
{
    const auto& pp=
        propositions.get<insight::VectorParameter>(
        selectedProposition);

    parameterRef()=pp;

    lineEdit->setText(
        QString::fromStdString(
            insight::valueToString(
                parameter()())) );
}
