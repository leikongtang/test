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

Q_DECLARE_METATYPE(InstalledApp)
Q_DECLARE_METATYPE(QVector<InstalledApp>)

#endif // INSTALLEDAPP_H
