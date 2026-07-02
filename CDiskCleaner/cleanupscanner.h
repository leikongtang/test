#ifndef CLEANUPSCANNER_H
#define CLEANUPSCANNER_H

#include "cleanupcategory.h"

#include <QVector>

class CleanupScanner
{
public:
    static QVector<CleanupCategory> defaultCategories();
    static qint64 calculateDirectorySize(const QString &path, bool *cancelled = nullptr);
    static qint64 scanCategorySize(const CleanupCategory &category, bool *cancelled = nullptr);
    static bool cleanCategory(const CleanupCategory &category, bool *cancelled = nullptr);
    static QString formatSize(qint64 bytes);
    static void getDiskSpaceInfo(const QString &drive, qint64 *totalBytes, qint64 *freeBytes);
};

#endif // CLEANUPSCANNER_H
