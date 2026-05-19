#ifndef STATSMODEL_H
#define STATSMODEL_H

#include <QAbstractTableModel>
#include <QList>

struct CleanCategory {
    QString name;
    QString description;
    QString icon;
    qint64 size;
    QStringList files;
    bool checked;
    bool safeToDelete;
    
    CleanCategory() : size(0), checked(true), safeToDelete(true) {}
};

class StatsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit StatsModel(QObject *parent = nullptr);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    
    void setCategories(const QList<CleanCategory> &categories);
    QList<CleanCategory> getCategories() const;
    qint64 getTotalSize() const;
    qint64 getSelectedSize() const;
    
private:
    QList<CleanCategory> categories;
    QString formatSize(qint64 bytes) const;
};

#endif // STATSMODEL_H
