#ifndef CLEANUPCATEGORY_H
#define CLEANUPCATEGORY_H

#include <QString>

struct CleanupCategory
{
    enum class Id {
        UserTemp,
        WindowsTemp,
        RecycleBin,
        Prefetch,
        ThumbnailCache,
        WindowsUpdateCache,
        DeliveryOptimization
    };

    Id id;
    QString name;
    QString description;
    QString path;
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
};

#endif // CLEANUPCATEGORY_H
