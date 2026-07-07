#ifndef APPUNINSTALLER_H
#define APPUNINSTALLER_H

#include "installedapp.h"

#include <QVector>

struct UninstallResult
{
    bool success;
    QString message;

    UninstallResult()
        : success(false)
    {
    }
};

class AppUninstaller
{
public:
    static QVector<InstalledApp> enumerateInstalledApps();
    static UninstallResult uninstallApp(const InstalledApp &app, bool force);
    static QString formatSize(qint64 bytes);

private:
    static InstalledApp readAppFromRegistryKey(const QString &rootPath, const QString &subKey);
    static QString readRegistryString(const QString &keyPath, const QString &valueName);
    static bool terminateRelatedProcesses(const InstalledApp &app);
    static bool runUninstallCommand(const QString &command, int timeoutMs);
    static bool removeInstallDirectory(const QString &path);
    static bool removeRegistryKey(const QString &keyPath);
    static bool isProtectedApp(const InstalledApp &app);
};

#endif // APPUNINSTALLER_H
