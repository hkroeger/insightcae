#ifndef DRAWINGVIEWSMODEL_H
#define DRAWINGVIEWSMODEL_H

#include <QAbstractListModel>

#include "cadpostprocactions/drawingexport.h"

#include "viewdefinitiondialog.h"

std::string generateViewDefinitionExpression
    (const DrawingViewDefinition& vd);

class DrawingViewsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit DrawingViewsModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void appendView(const DrawingViewDefinition& vd);
    DrawingViewDefinition view(const QModelIndex& idx);
    void editView(const QModelIndex& idx, const DrawingViewDefinition& vd);
    void removeView(const QModelIndex& idx);

    const std::vector<DrawingViewDefinition>&
    viewDefinitions() const;

private:
    std::vector<DrawingViewDefinition> viewDefinitions_;

};

#endif // DRAWINGVIEWSMODEL_H
