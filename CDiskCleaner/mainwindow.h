#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cleanupcategory.h"

#include <QMainWindow>
#include <QVector>

class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QThread;
class CleanupWorker;

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

private:
    void setupUi();
    void setupWorker();
    void setUiBusy(bool busy);
    void updateCategoryRow(int row, const CleanupCategory &category);
    void updateSummary();
    QVector<CleanupCategory> collectSelectedCategories() const;

    QTableWidget *m_categoryTable;
    QLabel *m_diskTotalLabel;
    QLabel *m_diskUsedLabel;
    QLabel *m_diskFreeLabel;
    QLabel *m_scanResultLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_scanButton;
    QPushButton *m_cleanButton;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    QPushButton *m_refreshDiskButton;

    QVector<CleanupCategory> m_categories;
    QThread *m_workerThread;
    CleanupWorker *m_worker;
    bool m_isBusy;
    qint64 m_totalScannedBytes;
};

#endif // MAINWINDOW_H
