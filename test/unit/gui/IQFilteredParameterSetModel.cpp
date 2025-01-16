
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QDialog>
#include <QTreeView>
#include <QVBoxLayout>

#include "iqparametersetmodel.h"
#include "iqfilteredparametersetmodel.h"
#include "sampleparameterset.h"
#include "cadfeatures/line.h"

#define TEST_BASE_MODEL

using namespace insight;

int main(int argc, char*argv[])
{
    auto ps = TestPS::defaultParameters();

    auto& skp = ps->get<insight::CADSketchParameter>("outline");
    auto sk = skp.featureGeometryRef();

    /*auto wallDeflParam=std::make_unique<insight::ParameterSet>();
    wallDeflParam->insert("value", std::make_unique<insight::DoubleParameter>(1.33, "some dbl"));
    auto doorDeflParam = ParameterSet::create();

    SelectableSubsetParameter::Entries sele;
    sele.emplace("wall", std::move(wallDeflParam));
    sele.emplace("door", std::move(doorDeflParam));

    auto deflGeoP=ParameterSet::create();
    deflGeoP->insert(
        "type",
        std::make_unique<SelectableSubsetParameter>(
            "wall", std::move(sele), "") );*/

    auto deflGeoP=SubPS::defaultParameters();

    auto deflGeoP2=ParameterSet::create();
    deflGeoP2->insert("intval", std::make_unique<insight::IntParameter>(5, "some int"));

    auto p1=insight::cad::SketchPoint::create(sk->plane(), 0, 0);
    p1->changeDefaultParameters(*deflGeoP2);
    sk->insertGeometry(p1);
    auto p2=insight::cad::SketchPoint::create(sk->plane(), 10, 0);
    p2->changeDefaultParameters(*deflGeoP2);
    sk->insertGeometry(p2);
    auto l = insight::cad::Line::create(p1, p2);
    l->changeDefaultParameters(*deflGeoP);
    sk->insertGeometry(l);

    TestPS::Parameters p(*ps);

    auto L=ps->get<CADSketchParameter>("outline").featureGeometry().get<insight::cad::Line>(2);
    SubPS::Parameters sp(L->parameters());
    auto wp=boost::get<SubPS::Parameters::type_wall_type>(&sp.type);
    std::cout<<"path="<<wp->L2.parameterPath<<std::endl;

    IQParameterSetModel baseForModelToBeTested(
        ps->cloneParameterSet(), *TestPS::defaultParameters());

    // QAbstractItemModelTester tester(
    //     &baseForModelToBeTested,
    //     QAbstractItemModelTester::FailureReportingMode::Fatal );

#ifndef TEST_BASE_MODEL
    IQFilteredParameterSetModel modelToBeTested(
        {
         "mesh/refinementZones",
         "operation"
        });
    modelToBeTested.setSourceModel(&baseForModelToBeTested);

    QAbstractItemModelTester tester(
        &modelToBeTested,
        QAbstractItemModelTester::FailureReportingMode::Fatal );
#endif


    if (argc<=1 || std::string(argv[1])!="nogui")
    {
        QApplication app(argc, argv);
        QDialog dlg;
        auto *l=new QVBoxLayout;
        auto *tv=new QTreeView;
        tv->setItemDelegate(new IQParameterGridViewSelectorDelegate);
        l->addWidget(tv);
#ifdef TEST_BASE_MODEL
        tv->setModel(&baseForModelToBeTested);
#else
        tv->setModel(&modelToBeTested);
#endif
        tv->setContextMenuPolicy(Qt::CustomContextMenu);
        tv->setAlternatingRowColors(true);
        tv->setDragDropMode(QAbstractItemView::DragDrop);
        tv->setDefaultDropAction(Qt::MoveAction);
        QObject::connect(
            tv, &QTreeView::customContextMenuRequested,
            [tv](const QPoint& p)
            {
                IQParameterSetModel::contextMenu(
                    tv,
                    tv->indexAt(p),
                    p );
            }
            );
        dlg.setLayout(l);
        dlg.exec();
    }
}
