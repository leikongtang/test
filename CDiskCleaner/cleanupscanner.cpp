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

CleanupCategory CleanupScanner::makeCategory(CleanupCategory::Id id,
                                             const QString &group,
                                             const QString &name,
                                             const QString &description,
                                             const QStringList &paths,
                                             bool requiresAdmin,
                                             bool selected)
{
    CleanupCategory category;
    category.id = id;
    category.group = group;
    category.name = name;
    category.description = description;
    category.paths = paths;
    category.path = paths.isEmpty() ? QString() : paths.first();
    category.requiresAdmin = requiresAdmin;
    category.selected = selected;
    return category;
}

QStringList CleanupScanner::discoverFirefoxCachePaths()
{
    QStringList result;
    const QString appData = qEnvironmentVariable("APPDATA");
    if (appData.isEmpty()) {
        return result;
    }

    QDir profilesDir(appData + QStringLiteral("/Mozilla/Firefox/Profiles"));
    if (!profilesDir.exists()) {
        return result;
    }

    const QFileInfoList profiles = profilesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &profile : profiles) {
        const QString cache2 = profile.absoluteFilePath() + QStringLiteral("/cache2");
        const QString startupCache = profile.absoluteFilePath() + QStringLiteral("/startupCache");
        if (QDir(cache2).exists()) {
            result.append(cache2);
        }
        if (QDir(startupCache).exists()) {
            result.append(startupCache);
        }
    }

    return result;
}

QStringList CleanupScanner::discoverWpsCachePaths()
{
    QStringList result;
    const QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (localAppData.isEmpty()) {
        return result;
    }

    QDir kingsoftDir(localAppData + QStringLiteral("/Kingsoft"));
    if (!kingsoftDir.exists()) {
        return result;
    }

    const QFileInfoList wpsRoots = kingsoftDir.entryInfoList(
        QStringList{QStringLiteral("WPS Office")}, QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo &wpsRoot : wpsRoots) {
        QDirIterator it(wpsRoot.absoluteFilePath(), QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo info = it.fileInfo();
            if (info.fileName().compare(QStringLiteral("cache"), Qt::CaseInsensitive) == 0
                || info.fileName().compare(QStringLiteral("cachebak"), Qt::CaseInsensitive) == 0) {
                result.append(info.absoluteFilePath());
            }
        }
    }

    return result;
}

QVector<CleanupCategory> CleanupScanner::defaultCategories()
{
    QVector<CleanupCategory> categories;
    const QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    const QString appData = qEnvironmentVariable("APPDATA");

    categories.append(makeCategory(
        CleanupCategory::Id::UserTemp,
        QStringLiteral("系统"),
        QStringLiteral("用户临时文件"),
        QStringLiteral("当前用户 Temp 目录中的临时文件"),
        {QStandardPaths::writableLocation(QStandardPaths::TempLocation)},
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::WindowsTemp,
        QStringLiteral("系统"),
        QStringLiteral("Windows 临时文件"),
        QStringLiteral("系统 Temp 目录中的临时文件"),
        {QStringLiteral("C:/Windows/Temp")},
        true,
        true));

    CleanupCategory recycleBin;
    recycleBin.id = CleanupCategory::Id::RecycleBin;
    recycleBin.group = QStringLiteral("系统");
    recycleBin.name = QStringLiteral("回收站");
    recycleBin.description = QStringLiteral("清空所有驱动器的回收站");
    recycleBin.requiresAdmin = false;
    recycleBin.selected = true;
    categories.append(recycleBin);

    categories.append(makeCategory(
        CleanupCategory::Id::Prefetch,
        QStringLiteral("系统"),
        QStringLiteral("预读取文件"),
        QStringLiteral("Windows Prefetch 预读取缓存"),
        {QStringLiteral("C:/Windows/Prefetch")},
        true,
        false));

    categories.append(makeCategory(
        CleanupCategory::Id::ThumbnailCache,
        QStringLiteral("系统"),
        QStringLiteral("缩略图缓存"),
        QStringLiteral("资源管理器缩略图数据库缓存"),
        {localAppData + QStringLiteral("/Microsoft/Windows/Explorer")},
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::WindowsUpdateCache,
        QStringLiteral("系统"),
        QStringLiteral("Windows 更新缓存"),
        QStringLiteral("SoftwareDistribution 下载缓存（需管理员权限）"),
        {QStringLiteral("C:/Windows/SoftwareDistribution/Download")},
        true,
        false));

    categories.append(makeCategory(
        CleanupCategory::Id::DeliveryOptimization,
        QStringLiteral("系统"),
        QStringLiteral("传递优化文件"),
        QStringLiteral("Windows 更新传递优化缓存"),
        {QStringLiteral("C:/Windows/SoftwareDistribution/DeliveryOptimization")},
        true,
        false));

    categories.append(makeCategory(
        CleanupCategory::Id::ChromeCache,
        QStringLiteral("软件"),
        QStringLiteral("Chrome 浏览器缓存"),
        QStringLiteral("Google Chrome 网页缓存、代码缓存与 GPU 缓存"),
        {
            localAppData + QStringLiteral("/Google/Chrome/User Data/Default/Cache"),
            localAppData + QStringLiteral("/Google/Chrome/User Data/Default/Code Cache"),
            localAppData + QStringLiteral("/Google/Chrome/User Data/Default/GPUCache"),
            localAppData + QStringLiteral("/Google/Chrome/User Data/Default/Service Worker/CacheStorage")
        },
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::EdgeCache,
        QStringLiteral("软件"),
        QStringLiteral("Edge 浏览器缓存"),
        QStringLiteral("Microsoft Edge 网页缓存、代码缓存与 GPU 缓存"),
        {
            localAppData + QStringLiteral("/Microsoft/Edge/User Data/Default/Cache"),
            localAppData + QStringLiteral("/Microsoft/Edge/User Data/Default/Code Cache"),
            localAppData + QStringLiteral("/Microsoft/Edge/User Data/Default/GPUCache"),
            localAppData + QStringLiteral("/Microsoft/Edge/User Data/Default/Service Worker/CacheStorage")
        },
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::FirefoxCache,
        QStringLiteral("软件"),
        QStringLiteral("Firefox 浏览器缓存"),
        QStringLiteral("Mozilla Firefox 各配置文件的 cache2 与 startupCache"),
        discoverFirefoxCachePaths(),
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::WeChatCache,
        QStringLiteral("软件"),
        QStringLiteral("微信垃圾文件"),
        QStringLiteral("微信日志、临时文件与更新下载缓存"),
        {
            appData + QStringLiteral("/Tencent/WeChat/log"),
            localAppData + QStringLiteral("/Tencent/WeChat/Temp"),
            appData + QStringLiteral("/Tencent/WeChat/All Users/config/Applet"),
            localAppData + QStringLiteral("/Tencent/WeChat/update")
        },
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::QQCache,
        QStringLiteral("软件"),
        QStringLiteral("QQ 垃圾文件"),
        QStringLiteral("QQ 临时文件、日志与图片缓存"),
        {
            localAppData + QStringLiteral("/Tencent/QQ/Temp"),
            appData + QStringLiteral("/Tencent/Logs"),
            localAppData + QStringLiteral("/Tencent/QQGuild/Temp"),
            localAppData + QStringLiteral("/Tencent/QQNT/Temp")
        },
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::DingTalkCache,
        QStringLiteral("软件"),
        QStringLiteral("钉钉垃圾文件"),
        QStringLiteral("钉钉日志与本地缓存"),
        {
            appData + QStringLiteral("/DingTalk/log"),
            localAppData + QStringLiteral("/DingTalk/log"),
            localAppData + QStringLiteral("/DingTalk/cache")
        },
        false,
        true));

    categories.append(makeCategory(
        CleanupCategory::Id::WpsCache,
        QStringLiteral("软件"),
        QStringLiteral("WPS Office 缓存"),
        QStringLiteral("WPS 本地缓存与备份缓存目录"),
        discoverWpsCachePaths(),
        false,
        true));

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

    qint64 totalSize = 0;
    const QStringList pathList = category.effectivePaths();
    for (const QString &itemPath : pathList) {
        totalSize += calculateDirectorySize(itemPath, cancelled);
        if (cancelled && *cancelled) {
            break;
        }
    }
    return totalSize;
}

bool CleanupScanner::cleanDirectoryContents(const QString &path, bool *cancelled)
{
    if (path.isEmpty()) {
        return true;
    }

    QDir dir(path);
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
            subDir.removeRecursively();
        } else {
            QFile::remove(entry.absoluteFilePath());
        }
    }

    return true;
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

    const QStringList pathList = category.effectivePaths();
    if (pathList.isEmpty()) {
        return false;
    }

    bool allOk = true;
    for (const QString &itemPath : pathList) {
        if (!cleanDirectoryContents(itemPath, cancelled)) {
            allOk = false;
        }
        if (cancelled && *cancelled) {
            return false;
        }
    }

    return allOk;
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
