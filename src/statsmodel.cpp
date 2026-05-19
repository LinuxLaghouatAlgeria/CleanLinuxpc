#include "statsmodel.h"
#include <QIcon>

StatsModel::StatsModel(QObject *parent) : QAbstractTableModel(parent) {}

int StatsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return categories.size();
}

int StatsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 5; // ✔, النوع, الوصف, الحجم, الملفات
}

QVariant StatsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= categories.size())
        return QVariant();
    
    const CleanCategory &category = categories[index.row()];
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return QVariant(); // خانة التحديد
        case 1:
            return category.name;
        case 2:
            return category.description;
        case 3:
            return formatSize(category.size);
        case 4:
            return category.files.size();
        }
    } else if (role == Qt::CheckStateRole && index.column() == 0) {
        return category.checked ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::DecorationRole && index.column() == 1) {
        return QIcon(category.icon);
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 3 || index.column() == 4) {
            return Qt::AlignRight;
        }
        return Qt::AlignLeft;
    }
    
    return QVariant();
}

QVariant StatsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        QStringList headers = {tr("✔"), tr("النوع"), tr("الوصف"), tr("الحجم"), tr("الملفات")};
        return headers.value(section);
    }
    return QVariant();
}

bool StatsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= categories.size())
        return false;
    
    if (role == Qt::CheckStateRole && index.column() == 0) {
        categories[index.row()].checked = (value.toInt() == Qt::Checked);
        emit dataChanged(index, index, {role});
        return true;
    }
    
    return false;
}

Qt::ItemFlags StatsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    
    if (index.column() == 0) {
        flags |= Qt::ItemIsUserCheckable;
    }
    
    return flags;
}

void StatsModel::setCategories(const QList<CleanCategory> &newCategories)
{
    beginResetModel();
    categories = newCategories;
    endResetModel();
}

QList<CleanCategory> StatsModel::getCategories() const
{
    return categories;
}

qint64 StatsModel::getTotalSize() const
{
    qint64 total = 0;
    for (const auto &category : categories) {
        total += category.size;
    }
    return total;
}

qint64 StatsModel::getSelectedSize() const
{
    qint64 total = 0;
    for (const auto &category : categories) {
        if (category.checked) {
            total += category.size;
        }
    }
    return total;
}

QString StatsModel::formatSize(qint64 bytes) const
{
    const QStringList units = {"بايت", "كيلوبايت", "ميغابايت", "جيجابايت"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024 && unitIndex < units.size() - 1) {
        size /= 1024;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(QString::number(size, 'f', 2), units[unitIndex]);
}
