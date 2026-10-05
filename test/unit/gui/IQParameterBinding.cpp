
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>

#include <stdexcept>

#include "base/exception.h"
#include "base/parameters.h"
#include "iqparametersetmodel.h"
#include "iqparameterbinding.h"
#include "iqarrayelementselector.h"

using namespace insight;


#define CHECK(cond) \
    if (!(cond)) throw std::runtime_error( \
        std::string("check failed: " #cond " (line ") + std::to_string(__LINE__) + ")" );


// let deferred notifications happen
void flush()
{
    for (int i=0; i<3; ++i)
    {
        QCoreApplication::processEvents();
        QTest::qWait(5);
    }
}


std::unique_ptr<ParameterSet> testParameters()
{
    auto elem = ParameterSet::create("element");
    elem->insert("h", std::make_unique<DoubleParameter>(6., "height"));

    auto ps = ParameterSet::create();
    ps->insert("d", std::make_unique<DoubleParameter>(1.5, "a double"));
    ps->insert("s", std::make_unique<StringParameter>(std::string("abc"), std::string("a string")));
    ps->insert("v", std::make_unique<VectorParameter>(vec3(1,2,3), "a vector"));
    ps->insert("sel", std::make_unique<SelectionParameter>(
                          std::string("two"), SelectionParameter::ItemList{"one","two","three"}, "a selection"));
    ps->insert("arr", std::make_unique<ArrayParameter>(*elem, 2, "an array"));
    ps->insert("lab", std::make_unique<LabeledArrayParameter>(*elem, 1, "a labeled array"));
    return ps;
}


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    try
    {
        IQParameterSetModel model(testParameters());
        auto ps = [&model]() -> const ParameterSet& { return model.getParameterSet(); };

        QWidget container;
        auto *l = new QVBoxLayout(&container);

        IQParameterBindingGroup root(&model, nullptr);
        auto *dle = root.bindDouble("d");
        auto *sle = root.bindString("s");
        auto *selcb = root.bindSelection("sel");
        auto *vw = root.bindVector("v");
        l->addWidget(dle); l->addWidget(sle); l->addWidget(selcb); l->addWidget(vw);

        IQArrayElementSelector arrSel(&root, "arr", IQArrayElementSelector::List);
        IQParameterBindingGroup elem(&model, nullptr);
        elem.clearBasePath();
        QObject::connect(&arrSel, &IQArrayElementSelector::currentElementChanged, &elem,
                         [&elem](const std::string& p)
                         { if (p.empty()) elem.clearBasePath(); else elem.setBasePath(p); });
        auto *hle = elem.bindDouble("h");
        l->addWidget(&arrSel); l->addWidget(hle);

        flush();

        // initial values are read
        CHECK(dle->text()=="1.5");
        CHECK(sle->text()=="abc");
        CHECK(selcb->currentText()=="two");
        CHECK(arrSel.currentRow()==0);
        CHECK(elem.basePath()=="arr/0");
        CHECK(hle->text()=="6");

        // 1) widget edit => parameter & model notification
        {
            QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
            dle->setText("2.25");
            dle->setModified(true);
            Q_EMIT dle->editingFinished();
            CHECK(ps().get<DoubleParameter>("d")()==2.25);
            CHECK(spy.count()>0);

            selcb->setCurrentIndex(2);
            Q_EMIT selcb->activated(2);
            CHECK(ps().get<SelectionParameter>("sel").selection()=="three");

            hle->setText("3.5");
            hle->setModified(true);
            Q_EMIT hle->editingFinished();
            CHECK(ps().get<DoubleParameter>("arr/0/h")()==3.5);
        }

        // 2) external change => widget
        {
            ensureParameterWrapper(&model, "s");
            model.parameterRef("s").setDataFromString("xyz");
            ensureParameterWrapper(&model, "arr/0/h");
            dynamic_cast<DoubleParameter&>(model.parameterRef("arr/0/h")).set(7.);
            flush();
            CHECK(sle->text()=="xyz");
            CHECK(hle->text()=="7");
        }

        // 3) array element insertion/removal from both sides
        {
            ensureParameterWrapper(&model, "arr");
            dynamic_cast<ArrayParameter&>(model.parameterRef("arr")).appendEmpty();
            flush();
            arrSel.setCurrentRow(2);
            CHECK(elem.basePath()=="arr/2");

            modifyParameterAs<ArrayParameter>(
                &model, "arr", [](ArrayParameter& a) { a.eraseValue(2); } );
            flush();
            CHECK(arrSel.currentRow()==1); // clamped
            CHECK(elem.basePath()=="arr/1");
            CHECK(hle->isEnabled());
        }

        // 4) model reset => no stale parameters
        {
            auto nps = testParameters();
            nps->get<DoubleParameter>("d").set(9.);
            model.resetParameterValues(*nps);
            flush();
            CHECK(dle->text()=="9");

            dle->setText("4");
            dle->setModified(true);
            Q_EMIT dle->editingFinished();
            CHECK(model.getParameterSet().get<DoubleParameter>("d")()==4.);
        }

        // 5) unresolvable path => disabled
        {
            elem.setBasePath("arr/99");
            flush();
            CHECK(!hle->isEnabled());
        }

        std::cout<<"all checks passed"<<std::endl;
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return 1;
    }

    return 0;
}
