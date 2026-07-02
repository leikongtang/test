#ifndef CLEANUPWORKER_H
#define CLEANUPWORKER_H

#include "cleanupcategory.h"

#include <QObject>
#include <QVector>

class CleanupWorker : public QObject
{
    Q_OBJECT

public:
    enum class Mode {
        Scan,
        Clean
    };

    explicit CleanupWorker(QObject *parent = nullptr);

    void setCategories(const QVector<CleanupCategory> &categories);
    void setMode(Mode mode);
    void requestCancel();

public slots:
    void process();

signals:
    void categoryProgress(int index, const CleanupCategory &category);
    void finished(bool success, qint64 totalBytes, const QString &message);
    void errorOccurred(const QString &message);

private:
    QVector<CleanupCategory> m_categories;
    Mode m_mode;
    bool m_cancelled;
};

#endif // CLEANUPWORKER_H
