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
        emit scanFinished(AppUninstaller::enumerateInstalledApps());
        return;
    }

    const UninstallResult result = AppUninstaller::uninstallApp(m_targetApp, m_forceUninstall);
    emit uninstallFinished(result.success, result.message);
}
