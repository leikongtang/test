#ifndef CLEANUPCATEGORY_H
#define CLEANUPCATEGORY_H

#include <QString>
#include <QStringList>

struct CleanupCategory
{
    enum class Id {
        UserTemp,
        WindowsTemp,
        RecycleBin,
        Prefetch,
        ThumbnailCache,
        WindowsUpdateCache,
        DeliveryOptimization,
        ChromeCache,
        EdgeCache,
        FirefoxCache,
        WeChatCache,
        QQCache,
        DingTalkCache,
        WpsCache
    };

    Id id;
    QString group;
    QString name;
    QString description;
    QString path;
    QStringList paths;
    bool requiresAdmin;
    bool selected;
    qint64 sizeBytes;
    bool scanned;

    CleanupCategory()
        : id(Id::UserTemp)
        , requiresAdmin(false)
        , selected(true)
        , sizeBytes(0)
        , scanned(false)
    {
    }

    QStringList effectivePaths() const
    {
        if (!paths.isEmpty()) {
            return paths;
        }
        if (!path.isEmpty()) {
            return QStringList{path};
        }
        return QStringList();
    }
};

#endif // CLEANUPCATEGORY_H
