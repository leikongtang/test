#include "cleanupworker.h"
#include "cleanupscanner.h"

CleanupWorker::CleanupWorker(QObject *parent)
    : QObject(parent)
    , m_mode(Mode::Scan)
    , m_cancelled(false)
{
}

void CleanupWorker::setCategories(const QVector<CleanupCategory> &categories)
{
    m_categories = categories;
}

void CleanupWorker::setMode(CleanupWorker::Mode mode)
{
    m_mode = mode;
}

void CleanupWorker::requestCancel()
{
    m_cancelled = true;
}

void CleanupWorker::process()
{
    m_cancelled = false;
    qint64 totalBytes = 0;
    int processedCount = 0;

    for (int i = 0; i < m_categories.size(); ++i) {
        if (m_cancelled) {
            emit finished(false, totalBytes, QStringLiteral("操作已取消"));
            return;
        }

        CleanupCategory category = m_categories.at(i);
        if (!category.selected) {
            continue;
        }

        if (m_mode == Mode::Scan) {
            category.sizeBytes = CleanupScanner::scanCategorySize(category, &m_cancelled);
            category.scanned = true;
            totalBytes += category.sizeBytes;
        } else {
            const qint64 sizeBefore = category.scanned ? category.sizeBytes
                                                       : CleanupScanner::scanCategorySize(category, &m_cancelled);
            const bool ok = CleanupScanner::cleanCategory(category, &m_cancelled);
            if (!ok && category.requiresAdmin) {
                emit errorOccurred(QStringLiteral("%1 清理失败，可能需要管理员权限").arg(category.name));
            }
            totalBytes += sizeBefore;
            category.sizeBytes = 0;
            category.scanned = true;
        }

        ++processedCount;
        emit categoryProgress(i, category);
    }

    if (processedCount == 0) {
        emit finished(false, 0, QStringLiteral("请至少选择一项清理内容"));
        return;
    }

    const QString message = (m_mode == Mode::Scan)
                                ? QStringLiteral("扫描完成，共发现 %1 可清理空间")
                                      .arg(CleanupScanner::formatSize(totalBytes))
                                : QStringLiteral("清理完成，共释放 %1 空间")
                                      .arg(CleanupScanner::formatSize(totalBytes));

    emit finished(true, totalBytes, message);
}
