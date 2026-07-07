#include "applistmodel.h"
#include "appuninstaller.h"

#include <QFileIconProvider>
#include <QFileInfo>
#include <QColor>
#include <QFont>

namespace {

QString resolveIconPath(const QString &displayIcon)
{
    QString path = displayIcon.trimmed();
    if (path.isEmpty()) {
        return QString();
    }

    if (path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"')) && path.size() > 1) {
        path = path.mid(1, path.size() - 2);
    } else if (path.startsWith(QLatin1Char('"'))) {
        const int endQuote = path.indexOf(QLatin1Char('"'), 1);
        if (endQuote > 1) {
            path = path.mid(1, endQuote - 1);
        }
    }

    const int commaPos = path.lastIndexOf(QLatin1Char(','));
    if (commaPos > 0) {
        bool ok = false;
        path.mid(commaPos + 1).trimmed().toInt(&ok);
        if (ok) {
            path = path.left(commaPos).trimmed();
            if (path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"'))) {
                path = path.mid(1, path.size() - 2);
            }
        }
    }

    return path;
}

} // namespace

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
    return 6;
}

QIcon AppListModel::iconForApp(const InstalledApp &app) const
{
    if (app.displayIcon.isEmpty()) {
        return QIcon();
    }

    if (m_iconCache.contains(app.displayIcon)) {
        return m_iconCache.value(app.displayIcon);
    }

    const QString path = resolveIconPath(app.displayIcon);
    QIcon icon;
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        QFileIconProvider provider;
        icon = provider.icon(QFileInfo(path));
    }

    m_iconCache.insert(app.displayIcon, icon);
    return icon;
}

QVariant AppListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_apps.size()) {
        return QVariant();
    }

    const InstalledApp &app = m_apps.at(index.row());

    if (index.column() == 0 && role == Qt::DecorationRole) {
        return iconForApp(app);
    }

    if (index.column() == 4) {
        if (role == Qt::FontRole) {
            QFont font;
            font.setPointSize(11);
            return font;
        }
        if (role == Qt::ForegroundRole) {
            if (app.installLocation.isEmpty()) {
                return QVariant();
            }
            const bool exists = QFileInfo(app.installLocation).exists();
            return exists ? QColor(0, 102, 204) : QColor(128, 128, 128);
        }
        if (role == Qt::ToolTipRole) {
            if (app.installLocation.isEmpty()) {
                return QString();
            }
            return QStringLiteral("点击在资源管理器中打开\n%1").arg(app.installLocation);
        }
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (index.column()) {
    case 0:
        return QString();
    case 1:
        return app.displayName;
    case 2:
        return app.displayVersion;
    case 3:
        return app.publisher;
    case 4:
        return app.installLocation;
    case 5:
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
        return QStringLiteral("图标");
    case 1:
        return QStringLiteral("软件名称");
    case 2:
        return QStringLiteral("版本");
    case 3:
        return QStringLiteral("发布者");
    case 4:
        return QStringLiteral("安装路径");
    case 5:
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
    m_iconCache.clear();
    endResetModel();
}

InstalledApp AppListModel::appAt(int row) const
{
    if (row < 0 || row >= m_apps.size()) {
        return InstalledApp();
    }
    return m_apps.at(row);
}
