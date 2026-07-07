#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cleanupcategory.h"
#include "installedapp.h"

#include <QMainWindow>
#include <QVector>

class AppListModel;
class QCheckBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTabWidget;
class QTableView;
class QTableWidget;
class QThread;
class CleanupWorker;
class AppUninstallWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onScanClicked();
    void onCleanClicked();
    void onSelectAllClicked();
    void onSelectNoneClicked();
    void onCategoryProgress(int index, const CleanupCategory &category);
    void onWorkerFinished(bool success, qint64 totalBytes, const QString &message);
    void onWorkerError(const QString &message);
    void refreshDiskInfo();
    void onTabChanged(int index);

    void onRefreshAppsClicked();
    void onUninstallSearchChanged(const QString &text);
    void onUninstallClicked();
    void onForceUninstallClicked();
    void onAppScanBatchReady(const QVector<InstalledApp> &apps, int totalCount);
    void onAppScanFinished(int totalCount);
    void onAppUninstallFinished(bool success, const QString &message);

private:
    void setupUi();
    void setupCleanupTab(QWidget *tab);
    void setupUninstallTab(QWidget *tab);
    void setupWorker();
    void setupUninstallWorker();
    void setUiBusy(bool busy);
    void setUninstallLoading(bool loading);
    void startLoadAppsIfNeeded();
    void updateCategoryRow(int row, const CleanupCategory &category);
    void updateSummary();
    void rebuildFilteredApps();
    bool matchesAppFilter(const InstalledApp &app, const QString &keyword) const;
    QVector<CleanupCategory> collectSelectedCategories() const;
    InstalledApp currentSelectedApp() const;

    QTabWidget *m_tabWidget;

    QLabel *m_diskTotalLabel;
    QLabel *m_diskUsedLabel;
    QLabel *m_diskFreeLabel;
    QLabel *m_scanResultLabel;
    QLabel *m_statusLabel;
    QLabel *m_uninstallStatusLabel;
    QProgressBar *m_progressBar;
    QProgressBar *m_uninstallProgressBar;
    QPushButton *m_scanButton;
    QPushButton *m_cleanButton;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    QPushButton *m_refreshDiskButton;
    QPushButton *m_refreshAppsButton;
    QPushButton *m_uninstallButton;
    QPushButton *m_forceUninstallButton;
    QLineEdit *m_uninstallSearchEdit;
    QTableWidget *m_categoryTable;
    QTableView *m_appTableView;
    AppListModel *m_appModel;

    QVector<CleanupCategory> m_categories;
    QVector<InstalledApp> m_installedApps;
    QThread *m_workerThread;
    QThread *m_uninstallWorkerThread;
    CleanupWorker *m_worker;
    AppUninstallWorker *m_uninstallWorker;
    bool m_isBusy;
    bool m_isUninstallLoading;
    bool m_appsLoadStarted;
    bool m_appsLoadFinished;
    qint64 m_totalScannedBytes;
};

#endif // MAINWINDOW_H
