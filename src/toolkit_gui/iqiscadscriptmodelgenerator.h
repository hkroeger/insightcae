#ifndef IQISCADSCRIPTMODELGENERATOR_H
#define IQISCADSCRIPTMODELGENERATOR_H

#include "toolkit_gui_export.h"
#include "iqiscadmodelgenerator.h"

#include <memory>
#include <QMetaType>

#ifndef Q_MOC_RUN
#include "parser.h"
#endif




/**
 * @brief The ISCADParseResult struct
 * the result of parsing a model script.
 * Can be used to rebuild the model later without parsing again.
 */
struct TOOLKIT_GUI_EXPORT ISCADParseResult
{
    /**
     * @brief script
     * the script, from which this result was created
     */
    std::string script;

    insight::cad::ModelPtr model;
    insight::cad::parser::SyntaxElementDirectoryPtr syntaxElements;

    bool success = false;

    /**
     * @brief failpos, errorMsg, errorRange
     * location and description of a parsing error (if not success)
     */
    long failpos = -1;
    QString errorMsg;
    int errorRange = 1;
};

typedef std::shared_ptr<const ISCADParseResult> ISCADParseResultPtr;

Q_DECLARE_METATYPE(ISCADParseResultPtr)




class TOOLKIT_GUI_EXPORT IQISCADScriptModelGenerator
        : public IQISCADModelGenerator
{
    Q_OBJECT

public:
    /**
     * @brief The Task enum
     * Parse: only parse the script
     * Rebuild: parse and build all entities, including the previews of the postproc actions
     * Post: like Rebuild, but additionally write the output of all postproc actions
     */
    enum Task { Parse = 0, Rebuild = 1, Post = 2 };

public:
    /**
     * @brief parse
     * parse the script. Errors are not emitted but stored in the result.
     */
    ISCADParseResultPtr parse(const std::string& script);

    /**
     * @brief rebuild
     * report parsing errors of the given result and execute the given task
     * (nothing but the error report, if task is Parse).
     * Throws insight::cad::RebuildCancelException, if the rebuild was cancelled.
     */
    void rebuild(const ISCADParseResult& parsed, Task executeUntilTask);

Q_SIGNALS:
    void scriptError(long failpos, QString errorMsg, int range);

    void statusMessage(const QString& msg, double timeout=0);
    void statusProgress(int step, int totalSteps);
};


#endif // IQISCADSCRIPTMODELGENERATOR_H
