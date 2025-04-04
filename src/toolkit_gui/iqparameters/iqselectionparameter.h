#ifndef IQSELECTIONPARAMETER_H
#define IQSELECTIONPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/selectionparameter.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>


class TOOLKIT_GUI_EXPORT IQSelectionParameterInterface
{
public:
    virtual insight::SelectionParameterInterface& selectionParameterRef() =0;
    virtual const insight::SelectionParameterInterface& selectionParameter() const =0;

    QStringList selectionKeys() const
    {
        QStringList sel;
        auto keys=selectionParameter().selectionKeys();
        for ( auto& k: keys )
        {
            sel.append( QString::fromStdString(k) );
        }
        return sel;
    }
};





template<class SP>
class TOOLKIT_GUI_EXPORT IQSelectionParameterBase
    : public IQSpecializedParameter<SP>, public IQSelectionParameterInterface
{
    insight::SelectionParameterInterface& selectionParameterRef() override
    {
        return dynamic_cast<insight::SelectionParameterInterface&>(
            this->parameterRef());
    }

    const insight::SelectionParameterInterface& selectionParameter() const override
    {
        return dynamic_cast<const insight::SelectionParameterInterface&>(
            this->parameter());
    }

public:
  declareType(SP::typeName_());



  IQSelectionParameterBase
  (
      QObject* parent,
      IQParameterSetModel* psmodel,
      insight::Parameter* parameter,
      const insight::ParameterSet& defaultParameterSet
  )
    : IQSpecializedParameter<SP>(
            parent, psmodel, parameter, defaultParameterSet)
  {}



  QString valueText() const override
  {
      return QString::fromStdString(
          selectionParameter().selection() );
  }



  bool setValue(QVariant value) override
  {
      if (value.canConvert<int>())
      {
          selectionParameterRef()
              .setSelectionFromIndex(value.toInt());
          return true;
      }
      else if (value.canConvert<QString>())
      {
          selectionParameterRef()
              .setSelection(
                  value.toString().toStdString());
          return true;
      }
      return false;
  }



  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override
  {
      auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

      QHBoxLayout *layout2=new QHBoxLayout;
      QLabel *promptLabel = new QLabel("Selection:", editControlsContainer);
      layout2->addWidget(promptLabel);

      auto* selBox_=new QComboBox(editControlsContainer);
      auto keys=selectionParameter().selectionKeys();
      for (auto& k: keys)
      {
          auto qk=QString::fromStdString(k);
          auto ip=selectionParameter().iconPathForKey(k);
          if (ip.empty())
          {
              selBox_->addItem(qk);
          }
          else
          {
              selBox_->addItem(
                  QPixmap(QString::fromStdString(ip)),
                  qk);
          }
      }
      selBox_->setCurrentIndex( selectionParameter().selectionIndex() );
      selBox_->setIconSize(QSize(200,150));

      layout2->addWidget(selBox_);
      layout->addLayout(layout2);

      QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
      QObject::connect(
          apply, &QPushButton::pressed, [=]()
          {
              selectionParameterRef().setSelectionFromIndex(
                  selBox_->currentIndex() );
          }
          );
      layout->addWidget(apply);


      return layout;
  }

};


typedef IQSelectionParameterBase<insight::SelectionParameter> IQSelectionParameter;



class TOOLKIT_GUI_EXPORT IQSelectionDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT

public:
    IQSelectionDelegate(QObject * parent = 0);

    QWidget * createEditor(
        QWidget * parent,
        const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    void setEditorData(
        QWidget *editor,
        const QModelIndex &index ) const override;

    void setModelData(
        QWidget *editor,
        QAbstractItemModel *model,
        const QModelIndex &index ) const override;

private Q_SLOTS:
    void commitAndClose(int);
};



#endif // IQSELECTIONPARAMETER_H
