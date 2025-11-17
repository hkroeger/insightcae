
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QTreeView>
#include <QDialog>

#include "iqresultsetmodel.h"
#include "smiley_image.h"
#include "test_pdl.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {

        auto result = std::make_unique<ResultSet>(
            TestPDL::defaultParameters(),
            "Title", "Subtitle"
            );

        auto insertAll = [](ResultElementCollection& r) {
            // create result set with all elements
            r.insert<Comment>(
                "comment",
                "The formula $\\alpha=\\beta^2$ is important",
                "Description of a formula" );

            {
                PlotCurve c1(std::vector<double>{0.,1.,2.}, {2.,4.,16.}, "crv1",
                             "w l lc 1 t 'Curve 1'");
                PlotCurve c2(std::vector<double>{2.,3.,4.}, {2.,4.,16.}, "crv2",
                             "w l lc 2 dt 1 ax x1y2 t 'Curve 2'");
                r.insert<Chart>("someChart",
                                "$x$/m", "\\Psi",
                                PlotCurveList{c1, c2},
                                "Some plots", "",
                                "set y2tics"
                                );
            }

            r.insert<ScalarResult>("scalarresult",
                                   42, "the answer on everything", "", "J");

            r.insert<VectorResult>("vectorresult",
                                   vec3(1,2,3), "some random vector", "", "");

            r.insert<TabularResult>(
                "tabularresult",
                TabularResult::Headings{"Col_1", "Col_2"},
                TabularResult::Table{
                    {1., 2.},
                    {4., 6.}
                },
                "A table with some values", "", ""
                );

            r.insert<AttributeTableResult>(
                "attributeresulttable",
                AttributeTableResult::AttributeNames{
                                                     SimpleLatex("Attr_1"),
                                                     SimpleLatex("Attr_$\\alpha$") },
                AttributeTableResult::AttributeValues{1., 2.},
                "list of attributes", "", "N"
                );

            r.insert<Image>(
                "image",
                FileContainer(smileyImageBase64, "smiley.jpeg"),
                "a smiley", "" );

            r.insert<FileResult>(
                "fileresult",
                FileContainer(smileyImageBase64, "fileresult.jpeg"),
                "the smiley image file", "");

            {
                arma::mat phi, y, data;
                phi       << 0. << 90. << 180. << 270. << 360. << arma::endr;
                y   << 0.
                  << 1.
                  << 2.
                  << 3.
                  << arma::endr;

                data
                    << 0. << 0. << 0. << 0. << 0. << arma::endr
                    << 0. << 1. << 2. << 1. << 0. << arma::endr
                    << 0. << 1. << 3. << 1. << 0. << arma::endr
                    << 0. << 2. << 3. << 2. << 0. << arma::endr
                    ;
                r.insert<PolarContourChart>(
                    "contourchart",
                    "$r$", "cb", 3.,
                    PlotField(phi, y, data.t()),
                    "a polar contour chart", "",
                    std::vector<double>{1., 2., 3.}
                    );
            }

            r.insert<PolarChart>(
                "polarchart",
                "$r$",
                PlotCurveList{
                    PlotCurve(
                        std::vector<double>{0., 90., 180., 270., 360.},
                        std::vector<double>{0.,  5.,  10.,  12.,   3.},
                        "crv", "w l not"
                        )
                },
                "a polar chart", "", SI::deg
                );

        };

        insertAll(*result);


        // gnuplotpolarchartrenderer
        //     latexgnuplotrenderer
        //                     chartrenderer
        //                         fastgnuplotrenderer
        //                             gnuplotrenderer
        //                                 matplotlibrenderer
        //                                     polarchartrenderer
        // and all in subset
        auto &sec=result->insert<ResultSection>(
            "resultsection", "Example Subsection",
            "This subsection contains all elements from above the section again.");
        insertAll(sec);

        QApplication app(argc, argv);

        IQResultSetModel modelToBeTested( std::move(result) );

        QAbstractItemModelTester tester(
            &modelToBeTested,
            QAbstractItemModelTester::FailureReportingMode::Fatal);

        if (argc<=1 || std::string(argv[1])!="nogui")
        {
            QDialog dlg;
            auto *l=new QVBoxLayout;
            auto *tv=new QTreeView;
            l->addWidget(tv);
            tv->setModel(&modelToBeTested);
            tv->setContextMenuPolicy(Qt::CustomContextMenu);
            tv->setAlternatingRowColors(true);
            tv->setDragDropMode(QAbstractItemView::DragDrop);
            tv->setDefaultDropAction(Qt::MoveAction);

            dlg.setLayout(l);
            dlg.exec();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<"Failed: "<<ex.what()<<std::endl;
        return -1;
    }
    std::cout<<"Passed"<<std::endl;
    return 0;
}
