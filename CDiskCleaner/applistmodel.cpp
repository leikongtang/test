#include "applistmodel.h"
#include "appuninstaller.h"

AppListModel::AppListModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int AppListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_apps.size();
}

int AppListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return 5;
}

QVariant AppListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_apps.size()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    const InstalledApp &app = m_apps.at(index.row());
    switch (index.column()) {
    case 0:
        return app.displayName;
    case 1:
        return app.displayVersion;
    case 2:
        return app.publisher;
    case 3:
        return app.installLocation;
    case 4:
        return AppUninstaller::formatSize(app.estimatedSizeBytes);
    default:
        return QVariant();
    }
}

QVariant AppListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
    case 0:
        return QStringLiteral("软件名称");
    case 1:
        return QStringLiteral("版本");
    case 2:
        return QStringLiteral("发布者");
    case 3:
        return QStringLiteral("安装路径");
    case 4:
        return QStringLiteral("大小");
    default:
        return QVariant();
    }
}

void AppListModel::setApps(const QVector<InstalledApp> &apps)
{
    beginResetModel();
    m_apps = apps;
    endResetModel();
}

void AppListModel::appendApps(const QVector<InstalledApp> &apps)
{
    if (apps.isEmpty()) {
        return;
    }

    const int first = m_apps.size();
    const int last = first + apps.size() - 1;
    beginInsertRows(QModelIndex(), first, last);
    m_apps.append(apps);
    endInsertRows();
}

void AppListModel::clearApps()
{
    if (m_apps.isEmpty()) {
        return;
    }
    beginResetModel();
    m_apps.clear();
    endResetModel();
}

InstalledApp AppListModel::appAt(int row) const
{
    if (row < 0 || row >= m_apps.size()) {
        return InstalledApp();
    }
    return m_apps.at(row);
}
