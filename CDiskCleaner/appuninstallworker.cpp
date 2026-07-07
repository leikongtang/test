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
        QVector<InstalledApp> batch;
        batch.reserve(40);
        int totalCount = 0;

        AppUninstaller::enumerateInstalledAppsIncremental([&](const InstalledApp &app) {
            batch.append(app);
            ++totalCount;
            if (batch.size() >= 40) {
                emit scanBatchReady(batch, totalCount);
                batch.clear();
            }
        });

        if (!batch.isEmpty()) {
            emit scanBatchReady(batch, totalCount);
        }

        emit scanFinished(totalCount);
        return;
    }

    InstalledApp app = AppUninstaller::loadFullAppDetails(m_targetApp);
    const UninstallResult result = AppUninstaller::uninstallApp(app, m_forceUninstall);
    emit uninstallFinished(result.success, result.message);
}
