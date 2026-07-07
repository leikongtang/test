#include "appuninstallworker.h"
#include "appuninstaller.h"

AppUninstallWorker::AppUninstallWorker(QObject *parent)
    : QObject(parent)
    , m_mode(Mode::Scan)
    , m_forceUninstall(false)
{
}

void AppUninstallWorker::setMode(AppUninstallWorker::Mode mode)
{
    m_mode = mode;
}

void AppUninstallWorker::setTargetApp(const InstalledApp &app)
{
    m_targetApp = app;
}

void AppUninstallWorker::setForceUninstall(bool force)
{
    m_forceUninstall = force;
}

void AppUninstallWorker::process()
{
    if (m_mode == Mode::Scan) {
        QVector<InstalledApp> allApps;
        QVector<InstalledApp> batch;
        batch.reserve(40);

        AppUninstaller::enumerateInstalledAppsIncremental([&](const InstalledApp &app) {
            allApps.append(app);
            batch.append(app);
            if (batch.size() >= 40) {
                emit scanBatchReady(batch, allApps.size());
                batch.clear();
            }
        });

        if (!batch.isEmpty()) {
            emit scanBatchReady(batch, allApps.size());
        }

        emit scanFinished(allApps);
        return;
    }

    InstalledApp app = AppUninstaller::loadFullAppDetails(m_targetApp);
    const UninstallResult result = AppUninstaller::uninstallApp(app, m_forceUninstall);
    emit uninstallFinished(result.success, result.message);
}
