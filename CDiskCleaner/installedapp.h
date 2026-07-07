#ifndef INSTALLEDAPP_H
#define INSTALLEDAPP_H

#include <QString>

struct InstalledApp
{
    QString displayName;
    QString displayVersion;
    QString publisher;
    QString installLocation;
    QString uninstallString;
    QString quietUninstallString;
    QString registryPath;
    QString registryKey;
    qint64 estimatedSizeBytes;
    bool isSystemComponent;
    bool noRemove;

    InstalledApp()
        : estimatedSizeBytes(0)
        , isSystemComponent(false)
        , noRemove(false)
    {
    }

    bool canUninstall() const
    {
        return !noRemove && !displayName.isEmpty()
               && (!uninstallString.isEmpty() || !installLocation.isEmpty());
    }
};

#endif // INSTALLEDAPP_H
