
#include <QDebug>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>

#include "insightcaeapplication.h"

#include "qtextensions.h"



int main(int argc, char* argv[])
{
    InsightCAEApplication app ( argc, argv, "CADViewerTest" );

    auto lay=new QVBoxLayout;
    auto *btn=new QPushButton("Open File...");
    lay->addWidget(btn);
    auto *sbtn=new QPushButton("Save File...");
    lay->addWidget(sbtn);
    auto *s2btn=new QPushButton("Save File with options...");
    lay->addWidget(s2btn);
    auto *s3btn=new QPushButton("Open File with variants...");
    lay->addWidget(s3btn);
    auto *txt=new QLabel;
    lay->addWidget(txt);

    QWidget window;

    QObject::connect(
        btn, &QPushButton::clicked, btn,
        [txt,&window]()
        {
            if (auto fn = getFileName(
                    &window,
                    "Open file",
                    GetFileMode::Open,
                    {
                     {"txt", "Text file", false},
                     {"ist", "Insight input file", true},
                     {"*", "any file", false}
                    }
                    ))
            {
                txt->setText(fn.asQString());
            }
            else
            {
                txt->setText("(nothing selected)");
            }
        }
    );

    QObject::connect(
        sbtn, &QPushButton::clicked, sbtn,
        [txt,&window]()
        {
            if (auto fn = getFileName(
                    &window,
                    QString(),
                    GetFileMode::Save,
                    {
                        {"txt", "Text file", false},
                        {"ist", "Insight input file", true},
                        {"*", "any file", false}
                    }
                    ))
            {
                txt->setText(fn.asQString());
            }
            else
            {
                txt->setText("(nothing selected)");
            }
        }
    );


    QObject::connect(
        s2btn, &QPushButton::clicked, s2btn,
        [txt,&window]()
        {
            bool checkState;
            if (auto fn = getFileName(
                    &window,
                    QString(),
                    GetFileMode::Save,
                    {
                        {"txt", "Text file", false},
                        {"ist", "Insight input file", true},
                        {"*", "any file", false}
                    },
                    boost::none,
                    [&checkState](QGridLayout *fdl)
                    {
                        auto *cb = new QCheckBox;
                        cb->setText("Pack: embed externally referenced files into parameterset");
                        int last_row=fdl->rowCount(); // id of new row below
                        fdl->addWidget(cb, last_row, 0, 1, -1);

                        cb->setChecked(true);

                        QObject::connect(cb, &QCheckBox::destroyed, cb,
                                [&checkState,cb](){ checkState=cb->isChecked(); } );
                    }
                    ))
            {
                txt->setText(
                    fn.asQString()+" "+
                    (checkState?"checked":"unchecked")
                    );
            }
            else
            {
                txt->setText("(nothing selected)");
            }
        }
        );

    QObject::connect(
        s3btn, &QPushButton::clicked, s3btn,
        [txt,&window]()
        {
            if (auto fn = getFileName(
                    &window, "Please select file",
                    GetFileMode::Open,
                    {
                        { "step stp", "STEP model" },
                        { "igs iges", "IGES model" },
                        { "brep", "BREP model" }
                    }))
            {
                txt->setText(fn.asQString());
            }
            else
            {
                txt->setText("(nothing selected)");
            }
        }
        );

    window.setLayout(lay);

    window.show();

    return app.exec();
}
