#ifndef IQISCADSCRIPTMODELGENERATOR_H
#define IQISCADSCRIPTMODELGENERATOR_H

#include "insightcad_gui_export.h"
#include "iqiscadmodelgenerator.h"

#ifndef Q_MOC_RUN
#include "parser.h"
#endif


class INSIGHTCAD_GUI_EXPORT IQISCADScriptModelGenerator
        : public IQISCADModelGenerator
{
    Q_OBJECT

public:
    enum Task { Parse = 0, Rebuild = 1, Post = 2 };

public:
    insight::cad::parser::SyntaxElementDirectoryPtr
    generate(const std::string& script, Task executeUntilTask = Parse);

Q_SIGNALS:
    void scriptError(long failpos, QString errorMsg, int range);

    void statusMessage(const QString& msg, double timeout=0);
    void statusProgress(int step, int totalSteps);
};


#endif // IQISCADSCRIPTMODELGENERATOR_H
