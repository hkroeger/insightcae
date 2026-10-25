/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef INSIGHT_CAD_ISCADMODEL_H
#define INSIGHT_CAD_ISCADMODEL_H

#include <QPlainTextEdit>
#include <QTimer>

#include <functional>
#include <map>

#include "base/boost_include.h"
#include "iqiscadscriptjobscheduler.h"
#include "scriptpositionmap.h"
#include "iqiscadcompleter.h"


#ifndef Q_MOC_RUN
#include "parser.h"
#endif


class IQCADItemModel;
class IQISCADSyntaxHighlighter;
class IQISCADFoldingMargin;
struct IQISCADToken;
class IQCADModel3DViewer;

namespace insight { namespace cad {
class ParametricSketch;
class ConstrainedSketch;
} }



/**
 * the container for the CAD modeling data, is also the text editor widget
 */
class IQISCADModelScriptEdit
: public QPlainTextEdit
{
    Q_OBJECT
    
    friend class IQISCADMainWindow;
    friend class IQISCADFoldingMargin;

public:
    /**
     * a collapsible range of the script (in editor positions):
     * the argument list of a command or an in-situ submodel block
     */
    struct FoldRegion
    {
        int begin, end;
        /**
         * name of the command, empty for a submodel block
         */
        QString command;
        bool collapsed;
    };

protected:
    boost::filesystem::path filename_;
    IQISCADSyntaxHighlighter* highlighter_;
    IQISCADFoldingMargin* margin_;
    IQISCADCompleter* completer_;

    const int bgparseInterval=1000;
    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;

    /**
     * position conversion for the script, from which syn_elem_dir_ was created
     */
    ScriptPositionMap syn_elem_pos_;

    /**
     * collapsible ranges, sorted by begin position.
     * The positions are adapted to edits until the next parse result arrives.
     */
    std::vector<FoldRegion> foldRegions_;

    /**
     * the command, whose signature is currently displayed in the status bar
     */
    QString calltipCommand_;

    /**
     * all known command and function names
     */
    std::map<QString, IQISCADCompleter::CandidateKind> knownCommands_;
    void updateKnownCommands();

    /**
     * member names of a built feature,
     * accessible by ".", "$", "@", "!" and "%"
     */
    struct BuiltMembers
    {
        QStringList subfeatures, scalars, points, vectors, datums;
    };

    /**
     * members of the features, which were built during the last rebuild.
     * Keyed by symbol path, e.g. "m" or "m.instanceFinal".
     */
    std::map<QString, BuiltMembers> builtMembers_;

    /**
     * collect the members of all built features after a rebuild.
     * Must only be called while no job is running.
     */
    void harvestBuiltMembers();

    /**
     * true, if the current completion popup shows members (after an accessor)
     */
    bool popupShowsMembers_ = false;

    /**
     * the feature highlighted in the completion popup is displayed
     * in the viewer after a short delay (it might need to be built)
     */
    QTimer* exposeTimer_ = nullptr;

    /**
     * symbol chain of the feature to display (e.g. {"m", "instanceFinal", "s"})
     */
    QStringList pendingExposeChain_;

    /**
     * set, when a completion has been inserted (the popup closes before)
     */
    bool completionAccepted_ = false;

    bool unsaved_;

    IQCADItemModel* cur_model_;

    /**
     * @brief viewer_
     * the 3D viewer, in which the sketch editor is executed
     */
    IQCADModel3DViewer* viewer_;
    
    /**
     * @brief scheduler_
     * runs background parsing and model rebuilds
     */
    IQISCADScriptJobScheduler* scheduler_;
    
    bool skipPostprocActions_;

    QSize sizehint_;

    int fontSize_;
    bool hasBeenRebuilt_;

    /**
     * @brief inSelectionChanged_
     * guard against recursive execution of onEditorSelectionChanged
     */
    bool inSelectionChanged_;

    void setFontSize(int fontSize);
    
protected:
    void clearDerivedData();

    /**
     * emit focus for the feature. Errors during creation or display
     * are reported in the status bar.
     * @return true, if a feature was focussed
     */
    bool tryFocus(std::function<insight::cad::FeaturePtr()> getFeature);

    /**
     * focus the element under the cursor in the viewer, unfocus if there is none
     */
    void focusElementAtCursor();

    /**
     * the feature, which is denoted by the symbol chain
     * (e.g. {"m", "instanceFinal", "s"} for m.instanceFinal.s) at the given position.
     * Returns nullptr, if the base symbol is unknown.
     */
    insight::cad::FeaturePtr resolveFeature(const QStringList& chain, int pos) const;

    /**
     * an entry of the completion popup was highlighted
     */
    void onCompletionHighlighted(const QModelIndex& index);

    bool eventFilter(QObject *watched, QEvent *event) override;

    // ========== parse result based highlighting

    /**
     * update the tokens and fold ranges from a new parse result.
     * The new information is valid up to validChars.
     * Beyond, the previous information is kept.
     */
    void updateAnnotations(
        const insight::cad::parser::SyntaxElementDirectory& sd,
        const ScriptPositionMap& pm,
        int validChars );

    /**
     * adapt all positions to an edit
     */
    void onContentsChange(int pos, int removed, int added);

    /**
     * the parser position (byte offset) for the given editor position
     */
    long parserPosition(int editorPosition) const;

    // ========== folding

    int marginWidth() const;
    void marginPaintEvent(QPaintEvent *event);
    void marginMousePressEvent(QMouseEvent *event);
    void updateMarginWidth();
    void updateMarginArea(const QRect &rect, int dy);

    /**
     * index of the foldable region, which starts in the given block, -1 if none
     */
    int foldRegionStartingIn(const QTextBlock& block) const;
    bool isFoldable(const FoldRegion& r) const;

    /**
     * true, if the given position is on one of the lines, which are hidden,
     * when the region is collapsed
     */
    bool hidesPosition(const FoldRegion& r, int pos) const;

    /**
     * set the visibility of all blocks according to the collapsed state of the fold regions
     */
    void applyFolding();
    void setCollapsed(int regionIndex, bool collapsed);
    void setAllCollapsed(bool collapsed, const std::function<bool(const FoldRegion&)>& filter);

    /**
     * expand all regions, which hide the cursor
     */
    void revealCursor();

    // ========== completion

    /**
     * the identifier left of the cursor
     */
    QString identifierPrefix() const;
    bool cursorInCommentOrString() const;

    /**
     * name of the innermost command, whose argument list contains the given position.
     * Empty, if there is none.
     * Works on the text, i.e. also for incomplete commands.
     */
    QString enclosingCommand(int pos) const;

    /**
     * highlight all occurrences of the given identifier
     */
    void highlightOccurrences(const QString& word);

    /**
     * an accessor (".", "$", "@", "!" or "%") in front of the cursor
     * (respectively in front of the identifier prefix), applied to the symbol chain
     * (e.g. "m.instanceFinal" -> {"m", "instanceFinal"})
     */
    struct MemberContext
    {
        QChar accessor;
        QStringList chain;
        bool valid = false;
    };
    MemberContext memberContext(int pos) const;

    /**
     * innermost submodel block, which contains the given range. nullptr if there is none.
     */
    const FoldRegion* innermostBlock(int begin, int end) const;

    /**
     * true, if the defined symbol is visible at pos
     */
    bool isVisibleAt(const IQISCADToken& def, int pos) const;

    /**
     * what is known before building about the feature, to which a symbol is assigned
     */
    struct StaticScope
    {
        enum Kind { None, Block, Loop } kind = None;
        /**
         * the submodel block (Block) or the loop body (Loop)
         */
        const FoldRegion* block = nullptr;
    };
    StaticScope staticScopeOf(const IQISCADToken& def, const QString& text) const;

    std::vector<IQISCADCompleter::Candidate> memberCandidates(
        const MemberContext& mc, int pos, const QString& text ) const;

    std::vector<IQISCADCompleter::Candidate> completionCandidates(int pos) const;
    void showCompletion(bool explicitlyRequested);
    void insertCompletion(const QModelIndex& index);
    void updateCalltip();

    /**
     * the signature of the command (empty, if unknown)
     */
    static QString commandSignature(const QString& command);

    void keyPressEvent(QKeyEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

    /**
     * start the constrained sketch editor for the given parametric sketch
     * and replace its sketch script argument after editing
     */
    void editParametricSketch(std::shared_ptr<insight::cad::ParametricSketch> ps);

    /**
     * replace the text range [begin, end) by the script of the edited sketch.
     * The replacement is only done, if the text range still contains orgText.
     */
    void replaceParametricSketchScript(
        int begin, int end,
        const QString& orgText,
        bool hasScriptArgument,
        const insight::cad::ConstrainedSketch& sketch );

    /**
     * write the editor contents to the given file.
     * Displays an error message, if this fails.
     * @return true on success
     */
    bool writeScriptTo(const boost::filesystem::path& fn);

    inline void setFilename(const boost::filesystem::path& fn)
    {
        filename_=fn;
        emit updateTitle(filename_, false);
    }

    
public:
    IQISCADModelScriptEdit(QWidget* parent = 0, bool dobgparsing=true);
    ~IQISCADModelScriptEdit();
    
    void loadFile(const boost::filesystem::path& file);
    void setScript(const std::string& contents);

    inline const std::vector<FoldRegion>& foldRegions() const { return foldRegions_; }
    
    /**
     * add all defined planes to the clip plane menu
     */
//    void populateClipPlaneMenu(QMenu* clipplanemenu, QoccViewWidget* v);

//    void connectModelTree(QModelTree* mt) const;
    void setModel(IQCADItemModel* model);
    void setViewer(IQCADModel3DViewer* viewer);

    inline bool isUnsaved() const { return unsaved_; }

    virtual QSize sizeHint() const;

public slots:

    /**
     * some text has been selected. Highlight all identical occurrences
     */
    void onEditorSelectionChanged();

    /**
     * jump to definition of feature symbol
     */
    void jumpTo(const QString& featurename);

    /**
     * switch BG parsing on/off
     */
    void toggleBgParsing(int state);

    /**
     * a script was parsed (in background or during rebuild)
     */
    void onScriptParsed(ISCADParseResultPtr parseResult, IQISCADScriptModelGenerator::Task task);

    /**
     * execute sketch editor for selected insight::cad::Sketch
     * triggered from QAction (menu) through a signalMapper
     */
    void editSketch(QObject* sk_ptr);
    
    /**
     * open another insight::cad::ModelFeature in a new tab
     * triggered from QAction (menu) through a signalMapper
     */
    void editModel(QObject* mo_ptr);

    /**
     * switch writing of the Postproc Actions output during rebuild on/off
     * (previews are always built)
     */
    void toggleSkipPostprocActions(int state);
    
    void insertSectionCommentAtCursor();
    void insertFeatureAtCursor();
    void insertImportedModelAtCursor();
    void insertComponentNameAtCursor();
    void insertLibraryModelAtCursor();
    void insertDrawingAtCursor();

    void onIncreaseFontSize();
    void onDecreaseFontSize();

    void collapseAllSketches();
    void collapseAll();
    void expandAll();
//    /**
//     * display everything shaded
//     */
//    void allShaded();
    
//    /**
//     * display everything in wireframe
//     */
//    void allWireframe();

    
public Q_SLOTS:

    /**
     * save model under the file name from which is was loaded
     */
    bool saveModel();
    
    /**
     * save under new file name. Dialog will be displayed
     */
    bool saveModelAs();
    
    /**
     * trigger rebuilding of entire model
     */
    void rebuildModel(bool upToCursor=false);
    
    /**
     * rebuild only up to cursor
     */
    void rebuildModelUpToCursor();
    
    /**
     * clear the cache
     */
    void clearCache();
    
    /**
     * create a context menu in editor widget
     */
    void showEditorContextMenu(const QPoint&);

    /**
     * insert some text at the cursor
     */
    void insertTextAtCursor(const QString& snippet);


    void setUnsavedState(int i1=0, int i2=1, int i3=1);
    void unsetUnsavedState();

    void onScriptError(long failpos, QString errorMsg, int range, IQISCADScriptModelGenerator::Task task);

    void onCancelRebuild();

Q_SIGNALS:
    
    /**
     * user status informations
     */
    void displayStatusMessage(const QString&, double timeout=0);

    /**
     * @brief statusProgress
     * @param step
     * @param totalSteps
     * forwarded from parsing thread
     */
    void statusProgress(int step, int totalSteps);

    /**
     * change of model file name or save state (asterisk in front of name)
     */
    void updateTitle(const boost::filesystem::path& filepath, bool isUnSaved);
    
    /**
     * the model has been reevaluated and menu entries etc. may have changed:
     * 
     * - clip planes
     * 
     * errorState != 0 indicates that an error occurred and the model may be incomplete
     */
    void modelUpdated(int errorState =0);
    
    /**
     * open another model for editing
     */
    void openModel(const boost::filesystem::path& modelfile);

    /**
     * @brief clearData
     * clear all model data (e.g., before a new script is inserted during file load)
     */
    void clearData();

    void focus(insight::cad::FeaturePtr feat);
    void unfocus();

    void displayNeedsRefit();

//     // insert new features
//     void addFeature(const QString& sn, insight::cad::FeaturePtr sm, bool is_component);
//     void addDatum(const QString& sn, insight::cad::DatumPtr dm);
//     void addEvaluation(const QString& sn, insight::cad::PostprocActionPtr em, bool visible=false);
//     void addVariable(const QString& sn, insight::cad::parser::scalar sv);
//     void addVariable(const QString& sn, insight::cad::parser::vector vv);

};






#endif
