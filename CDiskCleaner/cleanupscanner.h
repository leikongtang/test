#ifndef CLEANUPSCANNER_H
#define CLEANUPSCANNER_H

#include "cleanupcategory.h"

#include <QVector>

class CleanupScanner
{
public:
    static QVector<CleanupCategory> defaultCategories();
    static void refreshDynamicCategoryPaths(QVector<CleanupCategory> &categories);
    static qint64 calculateDirectorySize(const QString &path, bool *cancelled = nullptr);
    static qint64 scanCategorySize(const CleanupCategory &category, bool *cancelled = nullptr);
    static bool cleanCategory(const CleanupCategory &category, bool *cancelled = nullptr);
    static QString formatSize(qint64 bytes);
    static void getDiskSpaceInfo(const QString &drive, qint64 *totalBytes, qint64 *freeBytes);

private:
    static CleanupCategory makeCategory(CleanupCategory::Id id,
                                        const QString &group,
                                        const QString &name,
                                        const QString &description,
                                        const QStringList &paths,
                                        bool requiresAdmin,
                                        bool selected);
    static QStringList discoverFirefoxCachePaths();
    static QStringList discoverWpsCachePaths();
    static QStringList discoverChromiumCachePaths(const QString &userDataRoot);
    static QStringList existingPaths(const QStringList &paths);
    static bool cleanDirectoryContents(const QString &path, bool *cancelled);
};

#endif // CLEANUPSCANNER_H
