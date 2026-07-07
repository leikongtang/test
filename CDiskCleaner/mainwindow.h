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
    void onSoftwareScanClicked();
    void onSoftwareCleanClicked();
    void onSoftwareSelectAllClicked();
    void onSoftwareSelectNoneClicked();
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
    void onAppScanFinished(const QVector<InstalledApp> &apps);
    void onAppUninstallFinished(bool success, const QString &message);
    void onAppTableClicked(const QModelIndex &index);

private:
    void setupUi();
    void setupCleanupTab(QWidget *tab);
    void setupSoftwareCacheTab(QWidget *tab);
    void setupUninstallTab(QWidget *tab);
    void setupWorker();
    void setupUninstallWorker();
    void setUiBusy(bool busy);
    void setUninstallLoading(bool loading);
    void startLoadAppsIfNeeded();
    void updateCategoryRow(int row, const CleanupCategory &category);
    void updateSoftwareCacheRow(int tableRow, const CleanupCategory &category);
    void updateCategoryViews(int categoryIndex, const CleanupCategory &category);
    void buildSoftwareCategoryIndices();
    void updateSummary();
    void updateSoftwareSummary();
    void rebuildFilteredApps();
    bool matchesAppFilter(const InstalledApp &app, const QString &keyword) const;
    QVector<CleanupCategory> collectSelectedCategories() const;
    QVector<CleanupCategory> collectSoftwareCategoriesForWorker() const;
    void syncSoftwareSelectionToDiskTable();
    InstalledApp currentSelectedApp() const;

    enum class CleanupContext {
        Disk,
        Software
    };

    QTabWidget *m_tabWidget;

    QLabel *m_diskTotalLabel;
    QLabel *m_diskUsedLabel;
    QLabel *m_diskFreeLabel;
    QLabel *m_scanResultLabel;
    QLabel *m_softwareScanResultLabel;
    QLabel *m_statusLabel;
    QLabel *m_softwareStatusLabel;
    QLabel *m_uninstallStatusLabel;
    QProgressBar *m_progressBar;
    QProgressBar *m_softwareProgressBar;
    QProgressBar *m_uninstallProgressBar;
    QPushButton *m_scanButton;
    QPushButton *m_cleanButton;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    QPushButton *m_softwareScanButton;
    QPushButton *m_softwareCleanButton;
    QPushButton *m_softwareSelectAllButton;
    QPushButton *m_softwareSelectNoneButton;
    QPushButton *m_refreshDiskButton;
    QPushButton *m_refreshAppsButton;
    QPushButton *m_uninstallButton;
    QPushButton *m_forceUninstallButton;
    QLineEdit *m_uninstallSearchEdit;
    QTableWidget *m_categoryTable;
    QTableWidget *m_softwareCacheTable;
    QTableView *m_appTableView;
    AppListModel *m_appModel;

    QVector<CleanupCategory> m_categories;
    QVector<int> m_softwareCategoryIndices;
    CleanupContext m_activeCleanupContext;
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
