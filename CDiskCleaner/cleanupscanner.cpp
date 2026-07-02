#include "cleanupscanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <string>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

QVector<CleanupCategory> CleanupScanner::defaultCategories()
{
    QVector<CleanupCategory> categories;

    CleanupCategory userTemp;
    userTemp.id = CleanupCategory::Id::UserTemp;
    userTemp.name = QStringLiteral("用户临时文件");
    userTemp.description = QStringLiteral("当前用户 Temp 目录中的临时文件");
    userTemp.path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    userTemp.requiresAdmin = false;
    userTemp.selected = true;
    categories.append(userTemp);

    CleanupCategory winTemp;
    winTemp.id = CleanupCategory::Id::WindowsTemp;
    winTemp.name = QStringLiteral("Windows 临时文件");
    winTemp.description = QStringLiteral("系统 Temp 目录中的临时文件");
    winTemp.path = QStringLiteral("C:/Windows/Temp");
    winTemp.requiresAdmin = true;
    winTemp.selected = true;
    categories.append(winTemp);

    CleanupCategory recycleBin;
    recycleBin.id = CleanupCategory::Id::RecycleBin;
    recycleBin.name = QStringLiteral("回收站");
    recycleBin.description = QStringLiteral("清空所有驱动器的回收站");
    recycleBin.path = QString();
    recycleBin.requiresAdmin = false;
    recycleBin.selected = true;
    categories.append(recycleBin);

    CleanupCategory prefetch;
    prefetch.id = CleanupCategory::Id::Prefetch;
    prefetch.name = QStringLiteral("预读取文件");
    prefetch.description = QStringLiteral("Windows Prefetch 预读取缓存");
    prefetch.path = QStringLiteral("C:/Windows/Prefetch");
    prefetch.requiresAdmin = true;
    prefetch.selected = false;
    categories.append(prefetch);

    CleanupCategory thumbnail;
    thumbnail.id = CleanupCategory::Id::ThumbnailCache;
    thumbnail.name = QStringLiteral("缩略图缓存");
    thumbnail.description = QStringLiteral("资源管理器缩略图数据库缓存");
    const QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    thumbnail.path = localAppData + QStringLiteral("/Microsoft/Windows/Explorer");
    thumbnail.requiresAdmin = false;
    thumbnail.selected = true;
    categories.append(thumbnail);

    CleanupCategory updateCache;
    updateCache.id = CleanupCategory::Id::WindowsUpdateCache;
    updateCache.name = QStringLiteral("Windows 更新缓存");
    updateCache.description = QStringLiteral("SoftwareDistribution 下载缓存（需管理员权限）");
    updateCache.path = QStringLiteral("C:/Windows/SoftwareDistribution/Download");
    updateCache.requiresAdmin = true;
    updateCache.selected = false;
    categories.append(updateCache);

    CleanupCategory deliveryOpt;
    deliveryOpt.id = CleanupCategory::Id::DeliveryOptimization;
    deliveryOpt.name = QStringLiteral("传递优化文件");
    deliveryOpt.description = QStringLiteral("Windows 更新传递优化缓存");
    deliveryOpt.path = QStringLiteral("C:/Windows/SoftwareDistribution/DeliveryOptimization");
    deliveryOpt.requiresAdmin = true;
    deliveryOpt.selected = false;
    categories.append(deliveryOpt);

    return categories;
}

qint64 CleanupScanner::calculateDirectorySize(const QString &path, bool *cancelled)
{
    if (path.isEmpty()) {
        return 0;
    }

    QDir dir(path);
    if (!dir.exists()) {
        return 0;
    }

    qint64 totalSize = 0;
    QDirIterator it(path, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        if (cancelled && *cancelled) {
            return totalSize;
        }

        it.next();
        const QFileInfo info = it.fileInfo();
        if (info.isFile() || info.isSymLink()) {
            totalSize += info.size();
        }
    }

    return totalSize;
}

qint64 CleanupScanner::scanCategorySize(const CleanupCategory &category, bool *cancelled)
{
#ifdef Q_OS_WIN
    if (category.id == CleanupCategory::Id::RecycleBin) {
        SHQUERYRBINFO rbInfo = {};
        rbInfo.cbSize = sizeof(SHQUERYRBINFO);
        if (SUCCEEDED(SHQueryRecycleBinW(nullptr, &rbInfo))) {
            return static_cast<qint64>(rbInfo.i64Size);
        }
        return 0;
    }
#endif

    return calculateDirectorySize(category.path, cancelled);
}

bool CleanupScanner::cleanCategory(const CleanupCategory &category, bool *cancelled)
{
#ifdef Q_OS_WIN
    if (category.id == CleanupCategory::Id::RecycleBin) {
        const int result = SHEmptyRecycleBinW(nullptr, nullptr,
                                              SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
        return result == S_OK;
    }
#endif

    if (category.path.isEmpty()) {
        return false;
    }

    QDir dir(category.path);
    if (!dir.exists()) {
        return true;
    }

    const QFileInfoList entries = dir.entryInfoList(
        QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    for (const QFileInfo &entry : entries) {
        if (cancelled && *cancelled) {
            return false;
        }

        if (entry.isDir()) {
            QDir subDir(entry.absoluteFilePath());
            if (!subDir.removeRecursively()) {
                // 部分文件可能被占用，跳过继续
            }
        } else {
            QFile::remove(entry.absoluteFilePath());
        }
    }

    return true;
}

QString CleanupScanner::formatSize(qint64 bytes)
{
    if (bytes < 0) {
        bytes = 0;
    }

    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;

    if (bytes >= static_cast<qint64>(gb)) {
        return QStringLiteral("%1 GB").arg(bytes / gb, 0, 'f', 2);
    }
    if (bytes >= static_cast<qint64>(mb)) {
        return QStringLiteral("%1 MB").arg(bytes / mb, 0, 'f', 2);
    }
    if (bytes >= static_cast<qint64>(kb)) {
        return QStringLiteral("%1 KB").arg(bytes / kb, 0, 'f', 2);
    }
    return QStringLiteral("%1 B").arg(bytes);
}

void CleanupScanner::getDiskSpaceInfo(const QString &drive, qint64 *totalBytes, qint64 *freeBytes)
{
    if (totalBytes) {
        *totalBytes = 0;
    }
    if (freeBytes) {
        *freeBytes = 0;
    }

#ifdef Q_OS_WIN
    ULARGE_INTEGER freeBytesAvailable = {};
    ULARGE_INTEGER totalNumberOfBytes = {};
    ULARGE_INTEGER totalNumberOfFreeBytes = {};

    const QString root = drive.endsWith(QStringLiteral(":"))
                             ? drive + QStringLiteral("\\")
                             : drive;
    const std::wstring rootPath = root.toStdWString();

    if (GetDiskFreeSpaceExW(rootPath.c_str(),
                            &freeBytesAvailable,
                            &totalNumberOfBytes,
                            &totalNumberOfFreeBytes)) {
        if (totalBytes) {
            *totalBytes = static_cast<qint64>(totalNumberOfBytes.QuadPart);
        }
        if (freeBytes) {
            *freeBytes = static_cast<qint64>(freeBytesAvailable.QuadPart);
        }
    }
#else
    Q_UNUSED(drive)
#endif
}
