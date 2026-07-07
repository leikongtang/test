#ifndef APPUNINSTALLWORKER_H
#define APPUNINSTALLWORKER_H

#include "installedapp.h"

#include <QObject>
#include <QVector>

class AppUninstallWorker : public QObject
{
    Q_OBJECT

public:
    enum class Mode {
        Scan,
        Uninstall
    };

    explicit AppUninstallWorker(QObject *parent = nullptr);

    void setMode(Mode mode);
    void setTargetApp(const InstalledApp &app);
    void setForceUninstall(bool force);

public slots:
    void process();

signals:
    void scanBatchReady(const QVector<InstalledApp> &apps, int totalCount);
    void scanFinished(const QVector<InstalledApp> &apps);
    void uninstallFinished(bool success, const QString &message);

private:
    Mode m_mode;
    InstalledApp m_targetApp;
    bool m_forceUninstall;
};

#endif // APPUNINSTALLWORKER_H
