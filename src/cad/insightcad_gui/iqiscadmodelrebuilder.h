#ifndef IQISCADMODELREBUILDER_H
#define IQISCADMODELREBUILDER_H

#include "insightcad_gui_export.h"

#include <set>
#include <string>

#include <QObject>
#include <QList>

#include "AIS_DisplayMode.hxx"

#include "boost/variant.hpp"

#include "cadtypes.h"

class IQCADItemModel;
class IQISCADModelGenerator;



struct SymbolsSnapshot
{
  std::set<std::string>
      scalars_, points_, directions_,
      features_,
      datums_, postprocactions_;
};




class INSIGHTCAD_GUI_EXPORT IQISCADModelRebuilder
        : public QObject
{
    Q_OBJECT

    IQCADItemModel *model_;
    QList<IQISCADModelGenerator*> generators_;

    /**
     * @brief symbolsSnapshot_
     * symbols before rebuild started
     */
    SymbolsSnapshot symbolsSnapshot_;

    void storeSymbolSnapshot();
    void connectGenerator(IQISCADModelGenerator* gen);
    void removeNonRecreatedSymbols();

public:
    IQISCADModelRebuilder(IQCADItemModel *model, QList<IQISCADModelGenerator*> gen, QObject *parent=nullptr);
    ~IQISCADModelRebuilder();

public Q_SLOTS:
    void onAddScalar     (const QString& name, insight::cad::ScalarPtr sv);
    void onAddVector     (const QString& name, insight::cad::VectorPtr vv, insight::cad::VectorVariableType vt);
    void onAddFeature    (const QString& name, insight::cad::FeaturePtr smp, bool is_component,
                          boost::variant<boost::blank,AIS_DisplayMode> ds = boost::blank() );
    void onAddDatum      (const QString& name, insight::cad::DatumPtr smp);
    void onAddEvaluation (const QString& name, insight::cad::PostprocActionPtr smp, bool visible=false);
};

#endif // IQISCADMODELREBUILDER_H
