#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include "installedapp.h"

#include <QAbstractTableModel>
#include <QVector>

class AppListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AppListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setApps(const QVector<InstalledApp> &apps);
    void appendApps(const QVector<InstalledApp> &apps);
    void clearApps();
    InstalledApp appAt(int row) const;

private:
    QVector<InstalledApp> m_apps;
};

#endif // APPLISTMODEL_H
