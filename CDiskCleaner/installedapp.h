#ifndef INSTALLEDAPP_H
#define INSTALLEDAPP_H

#include <QString>
#include <QVector>
#include <QMetaType>
#include <QtGlobal>

struct InstalledApp
{
    QString displayName;
    QString displayVersion;
    QString publisher;
    QString installLocation;
    QString displayIcon;
    QString uninstallString;
    QString quietUninstallString;
    QString registryPath;
    QString registryKey;
    QString packageFullName;
    qint64 estimatedSizeBytes;
    bool isSystemComponent;
    bool isAppxPackage;
    bool noRemove;

    InstalledApp()
        : estimatedSizeBytes(0)
        , isSystemComponent(false)
        , isAppxPackage(false)
        , noRemove(false)
    {
    }

    bool canUninstall() const
    {
        if (noRemove || displayName.isEmpty()) {
            return false;
        }
        if (isAppxPackage) {
            return !packageFullName.isEmpty();
        }
        return !uninstallString.isEmpty() || !installLocation.isEmpty();
    }
};

Q_DECLARE_METATYPE(InstalledApp)
Q_DECLARE_METATYPE(QVector<InstalledApp>)

#endif // INSTALLEDAPP_H
