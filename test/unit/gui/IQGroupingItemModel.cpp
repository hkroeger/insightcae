
#include <QAbstractItemModelTester>

#include <QMainWindow>
#include <QApplication>
#include <QTreeView>

#include "iqcaditemmodel.h"
#include "iqgroupingitemmodel.h"

#include "cadfeatures/sphere.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {
        auto cadmodel = std::make_shared<cad::Model>();

        // 0 M_PI
        // 1 deg
        cadmodel->addScalar("s1", cad::scalarconst(1.0)); // 2
        cadmodel->addScalar("s2", cad::scalarconst(2.0)); // 3
        cadmodel->addScalar("s2/s3", cad::scalarconst(3.0)); // 4
        cadmodel->addScalar("gr/s4", cad::scalarconst(4.0)); // 5
        cadmodel->addScalar("gr/s5", cad::scalarconst(5.0)); // 6
        cadmodel->addScalar("gr/sgr1/s6", cad::scalarconst(6.0)); // 7
        cadmodel->addScalar("gr/sgr1/sgr2/s7", cad::scalarconst(7.0)); // 8

        cadmodel->addModelstep("fgr/s1", cad::Sphere::create(cadmodel->lookupPoint("O"), cad::scalarconst(1.)), false);
        cadmodel->addModelstep("fgr/s2", cad::Sphere::create(cadmodel->lookupPoint("O"), cad::scalarconst(2.)), false);
        cadmodel->addModelstep("s3", cad::Sphere::create(cadmodel->lookupPoint("O"), cad::scalarconst(3.)), false);
        cadmodel->addModelstep("gr2/s4", cad::Sphere::create(cadmodel->lookupPoint("O"), cad::scalarconst(4.)), false);

        IQCADItemModel origModel(cadmodel);

        QAbstractItemModelTester tester(
            &origModel,
            QAbstractItemModelTester::FailureReportingMode::Fatal );

        IQGroupingItemModel groupedModelToBeTested;
        groupedModelToBeTested.setGroupColumn(IQCADItemModel::labelCol);
        groupedModelToBeTested.setSourceModel(&origModel);

        QAbstractItemModelTester testGrouped(
            &groupedModelToBeTested,
            QAbstractItemModelTester::FailureReportingMode::Fatal );

        bool skip=false;
        if (argc>1 && strcmp(argv[1], "nogui")==0) skip=true;

        if (!skip)
        {
            QApplication app(argc, argv);
            QMainWindow win;
            auto tv=new QTreeView;
            tv->setModel(&groupedModelToBeTested);
            win.setCentralWidget(tv);
            win.show();

            tv->expandAll();
            tv->resizeColumnToContents(1);
            auto s = win.size();
            s*=2;
            win.resize(s);

            return app.exec();
        }
    }

    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }

    return 0;
}
