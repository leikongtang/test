#include "mainwindow.h"

#include "applistmodel.h"
#include "appuninstaller.h"
#include "appuninstallworker.h"
#include "cleanupscanner.h"
#include "cleanupworker.h"

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_workerThread(nullptr)
    , m_uninstallWorkerThread(nullptr)
    , m_worker(nullptr)
    , m_uninstallWorker(nullptr)
    , m_isBusy(false)
    , m_isUninstallLoading(false)
    , m_appsLoadStarted(false)
    , m_appsLoadFinished(false)
    , m_totalScannedBytes(0)
    , m_activeCleanupContext(CleanupContext::Disk)
{
    m_categories = CleanupScanner::defaultCategories();
    buildSoftwareCategoryIndices();
    setupUi();
    setupWorker();
    setupUninstallWorker();
    refreshDiskInfo();
    updateSummary();
}

MainWindow::~MainWindow()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        if (m_worker) {
            m_worker->requestCancel();
        }
        m_workerThread->quit();
        m_workerThread->wait(3000);
    }

    if (m_uninstallWorkerThread && m_uninstallWorkerThread->isRunning()) {
        m_uninstallWorkerThread->quit();
        m_uninstallWorkerThread->wait(3000);
    }
}

void MainWindow::setupUi()
{
    const QString version = QApplication::applicationVersion();
    setWindowTitle(QStringLiteral("C盘清理工具 v%1").arg(version));
    resize(1020, 720);

    m_tabWidget = new QTabWidget(this);
    QWidget *cleanupTab = new QWidget(this);
    QWidget *softwareCacheTab = new QWidget(this);
    QWidget *uninstallTab = new QWidget(this);

    setupCleanupTab(cleanupTab);
    setupSoftwareCacheTab(softwareCacheTab);
    setupUninstallTab(uninstallTab);

    m_tabWidget->addTab(cleanupTab, QStringLiteral("磁盘清理"));
    m_tabWidget->addTab(softwareCacheTab, QStringLiteral("软件缓存"));
    m_tabWidget->addTab(uninstallTab, QStringLiteral("软件卸载"));

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    setCentralWidget(m_tabWidget);
}

void MainWindow::setupCleanupTab(QWidget *tab)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

    QHBoxLayout *diskLayout = new QHBoxLayout();
    QLabel *diskTitle = new QLabel(QStringLiteral("C: 盘空间"), tab);
    diskTitle->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 14px;"));
    m_diskTotalLabel = new QLabel(tab);
    m_diskUsedLabel = new QLabel(tab);
    m_diskFreeLabel = new QLabel(tab);
    m_refreshDiskButton = new QPushButton(QStringLiteral("刷新"), tab);
    connect(m_refreshDiskButton, &QPushButton::clicked, this, &MainWindow::refreshDiskInfo);

    diskLayout->addWidget(diskTitle);
    diskLayout->addSpacing(12);
    diskLayout->addWidget(m_diskTotalLabel);
    diskLayout->addSpacing(16);
    diskLayout->addWidget(m_diskUsedLabel);
    diskLayout->addSpacing(16);
    diskLayout->addWidget(m_diskFreeLabel);
    diskLayout->addStretch();
    diskLayout->addWidget(m_refreshDiskButton);

    m_categoryTable = new QTableWidget(m_categories.size(), 6, tab);
    m_categoryTable->setHorizontalHeaderLabels({
        QStringLiteral("选择"),
        QStringLiteral("分组"),
        QStringLiteral("清理项目"),
        QStringLiteral("说明"),
        QStringLiteral("路径"),
        QStringLiteral("大小")
    });
    m_categoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_categoryTable->verticalHeader()->setVisible(false);
    m_categoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_categoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int i = 0; i < m_categories.size(); ++i) {
        updateCategoryRow(i, m_categories.at(i));
    }

    QHBoxLayout *toolLayout = new QHBoxLayout();
    m_scanButton = new QPushButton(QStringLiteral("扫描"), tab);
    m_cleanButton = new QPushButton(QStringLiteral("清理选中项"), tab);
    m_selectAllButton = new QPushButton(QStringLiteral("全选"), tab);
    m_selectNoneButton = new QPushButton(QStringLiteral("全不选"), tab);
    m_scanResultLabel = new QLabel(QStringLiteral("可清理空间: --"), tab);
    m_scanResultLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));

    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_cleanButton, &QPushButton::clicked, this, &MainWindow::onCleanClicked);
    connect(m_selectAllButton, &QPushButton::clicked, this, &MainWindow::onSelectAllClicked);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &MainWindow::onSelectNoneClicked);

    toolLayout->addWidget(m_scanButton);
    toolLayout->addWidget(m_cleanButton);
    toolLayout->addWidget(m_selectAllButton);
    toolLayout->addWidget(m_selectNoneButton);
    toolLayout->addStretch();
    toolLayout->addWidget(m_scanResultLabel);

    m_progressBar = new QProgressBar(tab);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);

    m_statusLabel = new QLabel(QStringLiteral("就绪。建议先扫描，确认后再清理。"), tab);

    QLabel *tipLabel = new QLabel(
        QStringLiteral("提示：软件缓存清理前请关闭对应程序；带「需管理员」标记的项目请以管理员身份运行。"), tab);
    tipLabel->setStyleSheet(QStringLiteral("color: #666;"));

    mainLayout->addLayout(diskLayout);
    mainLayout->addWidget(m_categoryTable, 1);
    mainLayout->addLayout(toolLayout);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(tipLabel);
}

void MainWindow::setupSoftwareCacheTab(QWidget *tab)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

    m_softwareCacheTable = new QTableWidget(m_softwareCategoryIndices.size(), 5, tab);
    m_softwareCacheTable->setHorizontalHeaderLabels({
        QStringLiteral("选择"),
        QStringLiteral("软件"),
        QStringLiteral("说明"),
        QStringLiteral("路径"),
        QStringLiteral("大小")
    });
    m_softwareCacheTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_softwareCacheTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_softwareCacheTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_softwareCacheTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_softwareCacheTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_softwareCacheTable->verticalHeader()->setVisible(false);
    m_softwareCacheTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_softwareCacheTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int i = 0; i < m_softwareCategoryIndices.size(); ++i) {
        updateSoftwareCacheRow(i, m_categories.at(m_softwareCategoryIndices.at(i)));
    }

    QHBoxLayout *toolLayout = new QHBoxLayout();
    m_softwareScanButton = new QPushButton(QStringLiteral("扫描软件缓存"), tab);
    m_softwareCleanButton = new QPushButton(QStringLiteral("清理选中项"), tab);
    m_softwareSelectAllButton = new QPushButton(QStringLiteral("全选"), tab);
    m_softwareSelectNoneButton = new QPushButton(QStringLiteral("全不选"), tab);
    m_softwareScanResultLabel = new QLabel(QStringLiteral("可清理空间: --"), tab);
    m_softwareScanResultLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));

    connect(m_softwareScanButton, &QPushButton::clicked, this, &MainWindow::onSoftwareScanClicked);
    connect(m_softwareCleanButton, &QPushButton::clicked, this, &MainWindow::onSoftwareCleanClicked);
    connect(m_softwareSelectAllButton, &QPushButton::clicked, this, &MainWindow::onSoftwareSelectAllClicked);
    connect(m_softwareSelectNoneButton, &QPushButton::clicked, this, &MainWindow::onSoftwareSelectNoneClicked);

    toolLayout->addWidget(m_softwareScanButton);
    toolLayout->addWidget(m_softwareCleanButton);
    toolLayout->addWidget(m_softwareSelectAllButton);
    toolLayout->addWidget(m_softwareSelectNoneButton);
    toolLayout->addStretch();
    toolLayout->addWidget(m_softwareScanResultLabel);

    m_softwareProgressBar = new QProgressBar(tab);
    m_softwareProgressBar->setRange(0, 0);
    m_softwareProgressBar->setVisible(false);

    m_softwareStatusLabel = new QLabel(
        QStringLiteral("就绪。清理前请关闭对应软件，避免文件占用导致清理不完整。"), tab);

    QLabel *tipLabel = new QLabel(
        QStringLiteral("支持 Chrome/Edge/Firefox、微信/QQ/钉钉/企业微信/飞书、WPS、VS Code、Steam、npm/pip 等常见软件缓存。"), tab);
    tipLabel->setStyleSheet(QStringLiteral("color: #666;"));

    mainLayout->addWidget(m_softwareCacheTable, 1);
    mainLayout->addLayout(toolLayout);
    mainLayout->addWidget(m_softwareProgressBar);
    mainLayout->addWidget(m_softwareStatusLabel);
    mainLayout->addWidget(tipLabel);
}

void MainWindow::setupUninstallTab(QWidget *tab)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLabel *searchLabel = new QLabel(QStringLiteral("搜索软件:"), tab);
    m_uninstallSearchEdit = new QLineEdit(tab);
    m_uninstallSearchEdit->setPlaceholderText(QStringLiteral("输入软件名称、发布者或安装路径"));
    m_refreshAppsButton = new QPushButton(QStringLiteral("刷新列表"), tab);
    connect(m_uninstallSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onUninstallSearchChanged);
    connect(m_refreshAppsButton, &QPushButton::clicked, this, &MainWindow::onRefreshAppsClicked);

    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_uninstallSearchEdit, 1);
    searchLayout->addWidget(m_refreshAppsButton);

    m_appModel = new AppListModel(this);
    m_appTableView = new QTableView(tab);
    m_appTableView->setModel(m_appModel);
    m_appTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_appTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_appTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_appTableView->setAlternatingRowColors(true);
    m_appTableView->verticalHeader()->setVisible(false);
    m_appTableView->horizontalHeader()->setStretchLastSection(false);
    m_appTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_appTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_appTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_appTableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_appTableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_appTableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_appTableView->horizontalHeader()->setMinimumSectionSize(240);
    m_appTableView->setColumnWidth(0, 36);
    m_appTableView->setIconSize(QSize(24, 24));
    m_appTableView->verticalHeader()->setDefaultSectionSize(40);
    m_appTableView->setWordWrap(true);
    m_appTableView->setTextElideMode(Qt::ElideMiddle);
    m_appTableView->setSortingEnabled(false);
    m_appTableView->setMouseTracking(true);
    connect(m_appTableView, &QTableView::clicked, this, &MainWindow::onAppTableClicked);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_uninstallButton = new QPushButton(QStringLiteral("正常卸载"), tab);
    m_forceUninstallButton = new QPushButton(QStringLiteral("强制卸载"), tab);
    m_forceUninstallButton->setStyleSheet(QStringLiteral("color: #c50f1f;"));
    connect(m_uninstallButton, &QPushButton::clicked, this, &MainWindow::onUninstallClicked);
    connect(m_forceUninstallButton, &QPushButton::clicked, this, &MainWindow::onForceUninstallClicked);

    actionLayout->addWidget(m_uninstallButton);
    actionLayout->addWidget(m_forceUninstallButton);
    actionLayout->addStretch();

    m_uninstallProgressBar = new QProgressBar(tab);
    m_uninstallProgressBar->setRange(0, 0);
    m_uninstallProgressBar->setVisible(false);

    m_uninstallStatusLabel = new QLabel(QStringLiteral("切换到本页后将自动加载软件列表。"), tab);

    QLabel *tipLabel = new QLabel(
        QStringLiteral("提示：强制卸载会结束相关进程、删除安装目录并清理注册表项，请谨慎操作。系统组件已自动保护。"), tab);
    tipLabel->setStyleSheet(QStringLiteral("color: #666;"));

    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(m_appTableView, 1);
    mainLayout->addLayout(actionLayout);
    mainLayout->addWidget(m_uninstallProgressBar);
    mainLayout->addWidget(m_uninstallStatusLabel);
    mainLayout->addWidget(tipLabel);
}

void MainWindow::setupWorker()
{
    m_workerThread = new QThread(this);
    m_worker = new CleanupWorker();
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &CleanupWorker::process);
    connect(m_worker, &CleanupWorker::categoryProgress, this, &MainWindow::onCategoryProgress);
    connect(m_worker, &CleanupWorker::finished, this, &MainWindow::onWorkerFinished);
    connect(m_worker, &CleanupWorker::errorOccurred, this, &MainWindow::onWorkerError);
    connect(m_worker, &CleanupWorker::finished, m_workerThread, &QThread::quit);
}

void MainWindow::setupUninstallWorker()
{
    m_uninstallWorkerThread = new QThread(this);
    m_uninstallWorker = new AppUninstallWorker();
    m_uninstallWorker->moveToThread(m_uninstallWorkerThread);

    connect(m_uninstallWorkerThread, &QThread::started, m_uninstallWorker, &AppUninstallWorker::process);
    connect(m_uninstallWorker, &AppUninstallWorker::scanBatchReady, this, &MainWindow::onAppScanBatchReady, Qt::QueuedConnection);
    connect(m_uninstallWorker, &AppUninstallWorker::scanFinished, this, &MainWindow::onAppScanFinished, Qt::QueuedConnection);
    connect(m_uninstallWorker, &AppUninstallWorker::uninstallFinished, this, &MainWindow::onAppUninstallFinished, Qt::QueuedConnection);
    connect(m_uninstallWorker, &AppUninstallWorker::scanFinished, m_uninstallWorkerThread, &QThread::quit);
    connect(m_uninstallWorker, &AppUninstallWorker::uninstallFinished, m_uninstallWorkerThread, &QThread::quit);
}

void MainWindow::setUiBusy(bool busy)
{
    m_isBusy = busy;
    m_scanButton->setEnabled(!busy);
    m_cleanButton->setEnabled(!busy);
    m_selectAllButton->setEnabled(!busy);
    m_selectNoneButton->setEnabled(!busy);
    m_softwareScanButton->setEnabled(!busy);
    m_softwareCleanButton->setEnabled(!busy);
    m_softwareSelectAllButton->setEnabled(!busy);
    m_softwareSelectNoneButton->setEnabled(!busy);
    m_refreshDiskButton->setEnabled(!busy);
    m_categoryTable->setEnabled(!busy);
    m_softwareCacheTable->setEnabled(!busy);
    m_progressBar->setVisible(busy && m_activeCleanupContext == CleanupContext::Disk);
    m_softwareProgressBar->setVisible(busy && m_activeCleanupContext == CleanupContext::Software);
}

void MainWindow::setUninstallLoading(bool loading)
{
    m_isUninstallLoading = loading;
    m_refreshAppsButton->setEnabled(!loading);
    m_uninstallButton->setEnabled(!loading);
    m_forceUninstallButton->setEnabled(!loading);
    m_uninstallProgressBar->setVisible(loading);
}

void MainWindow::onTabChanged(int index)
{
    if (index == 2) {
        startLoadAppsIfNeeded();
    }
}

void MainWindow::buildSoftwareCategoryIndices()
{
    m_softwareCategoryIndices.clear();
    for (int i = 0; i < m_categories.size(); ++i) {
        if (m_categories.at(i).group == QStringLiteral("软件")) {
            m_softwareCategoryIndices.append(i);
        }
    }
}

void MainWindow::startLoadAppsIfNeeded()
{
    if (m_isUninstallLoading) {
        return;
    }

    if (m_appsLoadFinished) {
        return;
    }

    if (!m_installedApps.isEmpty()) {
        m_appsLoadFinished = true;
        rebuildFilteredApps();
        return;
    }

    onRefreshAppsClicked();
}

bool MainWindow::matchesAppFilter(const InstalledApp &app, const QString &keyword) const
{
    if (keyword.isEmpty()) {
        return true;
    }

    return app.displayName.contains(keyword, Qt::CaseInsensitive)
           || app.publisher.contains(keyword, Qt::CaseInsensitive)
           || app.installLocation.contains(keyword, Qt::CaseInsensitive);
}

void MainWindow::rebuildFilteredApps()
{
    const QString keyword = m_uninstallSearchEdit->text().trimmed();
    QVector<InstalledApp> filteredApps;
    filteredApps.reserve(m_installedApps.size());

    for (const InstalledApp &app : m_installedApps) {
        if (matchesAppFilter(app, keyword)) {
            filteredApps.append(app);
        }
    }

    m_appModel->setApps(filteredApps);
    m_uninstallStatusLabel->setText(
        QStringLiteral("共 %1 款软件，当前显示 %2 款。").arg(m_installedApps.size()).arg(filteredApps.size()));
}

InstalledApp MainWindow::currentSelectedApp() const
{
    const QModelIndex index = m_appTableView->currentIndex();
    if (!index.isValid()) {
        return InstalledApp();
    }
    return m_appModel->appAt(index.row());
}

void MainWindow::updateCategoryRow(int row, const CleanupCategory &category)
{
    QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_categoryTable->cellWidget(row, 0));
    if (!checkBox) {
        checkBox = new QCheckBox(this);
        checkBox->setChecked(category.selected);
        m_categoryTable->setCellWidget(row, 0, checkBox);
    } else {
        checkBox->setChecked(category.selected);
    }

    m_categoryTable->setItem(row, 1, new QTableWidgetItem(category.group));

    QString nameText = category.name;
    if (category.requiresAdmin) {
        nameText += QStringLiteral(" (需管理员)");
    }

    m_categoryTable->setItem(row, 2, new QTableWidgetItem(nameText));
    m_categoryTable->setItem(row, 3, new QTableWidgetItem(category.description));

    QString pathText;
    if (category.id == CleanupCategory::Id::RecycleBin) {
        pathText = QStringLiteral("系统回收站");
    } else {
        const QStringList pathList = category.effectivePaths();
        if (pathList.isEmpty()) {
            pathText = QStringLiteral("未检测到安装路径");
        } else if (pathList.size() == 1) {
            pathText = pathList.first();
        } else {
            pathText = QStringLiteral("%1 等 %2 个目录").arg(pathList.first()).arg(pathList.size());
        }
    }
    m_categoryTable->setItem(row, 4, new QTableWidgetItem(pathText));

    const QString sizeText = category.scanned
                                 ? CleanupScanner::formatSize(category.sizeBytes)
                                 : QStringLiteral("--");
    m_categoryTable->setItem(row, 5, new QTableWidgetItem(sizeText));
}

void MainWindow::updateSoftwareCacheRow(int tableRow, const CleanupCategory &category)
{
    if (tableRow < 0 || tableRow >= m_softwareCacheTable->rowCount()) {
        return;
    }

    QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_softwareCacheTable->cellWidget(tableRow, 0));
    if (!checkBox) {
        checkBox = new QCheckBox(this);
        checkBox->setChecked(category.selected);
        m_softwareCacheTable->setCellWidget(tableRow, 0, checkBox);
    } else {
        checkBox->setChecked(category.selected);
    }

    m_softwareCacheTable->setItem(tableRow, 1, new QTableWidgetItem(category.name));
    m_softwareCacheTable->setItem(tableRow, 2, new QTableWidgetItem(category.description));

    QString pathText;
    const QStringList pathList = category.effectivePaths();
    if (pathList.isEmpty()) {
        pathText = QStringLiteral("未检测到（可能未安装）");
    } else if (pathList.size() == 1) {
        pathText = pathList.first();
    } else {
        pathText = QStringLiteral("%1 等 %2 个目录").arg(pathList.first()).arg(pathList.size());
    }
    m_softwareCacheTable->setItem(tableRow, 3, new QTableWidgetItem(pathText));

    const QString sizeText = category.scanned
                                 ? CleanupScanner::formatSize(category.sizeBytes)
                                 : QStringLiteral("--");
    m_softwareCacheTable->setItem(tableRow, 4, new QTableWidgetItem(sizeText));
}

void MainWindow::updateCategoryViews(int categoryIndex, const CleanupCategory &category)
{
    if (categoryIndex >= 0 && categoryIndex < m_categories.size()) {
        m_categories[categoryIndex] = category;
        updateCategoryRow(categoryIndex, category);

        const int softwareRow = m_softwareCategoryIndices.indexOf(categoryIndex);
        if (softwareRow >= 0) {
            updateSoftwareCacheRow(softwareRow, category);
        }
    }
}

void MainWindow::updateSummary()
{
    qint64 selectedTotal = 0;
    for (int i = 0; i < m_categories.size(); ++i) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_categoryTable->cellWidget(i, 0));
        const bool selected = checkBox ? checkBox->isChecked() : m_categories.at(i).selected;
        if (selected && m_categories.at(i).scanned) {
            selectedTotal += m_categories.at(i).sizeBytes;
        }
    }

    if (m_totalScannedBytes > 0 || selectedTotal > 0) {
        m_scanResultLabel->setText(
            QStringLiteral("可清理空间: %1").arg(CleanupScanner::formatSize(selectedTotal)));
    } else {
        m_scanResultLabel->setText(QStringLiteral("可清理空间: --"));
    }
}

void MainWindow::updateSoftwareSummary()
{
    qint64 selectedTotal = 0;
    for (int i = 0; i < m_softwareCategoryIndices.size(); ++i) {
        const int categoryIndex = m_softwareCategoryIndices.at(i);
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_softwareCacheTable->cellWidget(i, 0));
        const bool selected = checkBox ? checkBox->isChecked() : m_categories.at(categoryIndex).selected;
        if (selected && m_categories.at(categoryIndex).scanned) {
            selectedTotal += m_categories.at(categoryIndex).sizeBytes;
        }
    }

    if (selectedTotal > 0) {
        m_softwareScanResultLabel->setText(
            QStringLiteral("可清理空间: %1").arg(CleanupScanner::formatSize(selectedTotal)));
    } else {
        m_softwareScanResultLabel->setText(QStringLiteral("可清理空间: --"));
    }
}

QVector<CleanupCategory> MainWindow::collectSelectedCategories() const
{
    QVector<CleanupCategory> result;
    for (int i = 0; i < m_categories.size(); ++i) {
        CleanupCategory category = m_categories.at(i);
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_categoryTable->cellWidget(i, 0));
        category.selected = checkBox ? checkBox->isChecked() : category.selected;
        result.append(category);
    }
    return result;
}

void MainWindow::syncSoftwareSelectionToDiskTable()
{
    for (int i = 0; i < m_softwareCategoryIndices.size(); ++i) {
        const int categoryIndex = m_softwareCategoryIndices.at(i);
        QCheckBox *softwareCheckBox = qobject_cast<QCheckBox *>(m_softwareCacheTable->cellWidget(i, 0));
        QCheckBox *diskCheckBox = qobject_cast<QCheckBox *>(m_categoryTable->cellWidget(categoryIndex, 0));
        if (softwareCheckBox && diskCheckBox) {
            diskCheckBox->setChecked(softwareCheckBox->isChecked());
        }
    }
}

QVector<CleanupCategory> MainWindow::collectSoftwareCategoriesForWorker() const
{
    QVector<CleanupCategory> result = m_categories;
    for (int i = 0; i < result.size(); ++i) {
        if (result.at(i).group == QStringLiteral("软件")) {
            const int softwareRow = m_softwareCategoryIndices.indexOf(i);
            QCheckBox *checkBox = softwareRow >= 0
                                       ? qobject_cast<QCheckBox *>(m_softwareCacheTable->cellWidget(softwareRow, 0))
                                       : nullptr;
            result[i].selected = checkBox ? checkBox->isChecked() : false;
        } else {
            result[i].selected = false;
        }
    }
    return result;
}

void MainWindow::refreshDiskInfo()
{
    qint64 totalBytes = 0;
    qint64 freeBytes = 0;
    CleanupScanner::getDiskSpaceInfo(QStringLiteral("C:"), &totalBytes, &freeBytes);

    const qint64 usedBytes = totalBytes - freeBytes;
    m_diskTotalLabel->setText(
        QStringLiteral("总容量: %1").arg(CleanupScanner::formatSize(totalBytes)));
    m_diskUsedLabel->setText(
        QStringLiteral("已用: %1").arg(CleanupScanner::formatSize(usedBytes)));
    m_diskFreeLabel->setText(
        QStringLiteral("可用: %1").arg(CleanupScanner::formatSize(freeBytes)));
    m_diskFreeLabel->setStyleSheet(QStringLiteral("color: #107c10; font-weight: bold;"));
}

void MainWindow::onScanClicked()
{
    if (m_isBusy) {
        return;
    }

    m_activeCleanupContext = CleanupContext::Disk;
    m_totalScannedBytes = 0;
    for (CleanupCategory &category : m_categories) {
        category.scanned = false;
        category.sizeBytes = 0;
    }

    const QVector<CleanupCategory> selected = collectSelectedCategories();
    m_worker->setCategories(selected);
    m_worker->setMode(CleanupWorker::Mode::Scan);
    setUiBusy(true);
    m_statusLabel->setText(QStringLiteral("正在扫描，请稍候..."));

    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    m_workerThread->start();
}

void MainWindow::onCleanClicked()
{
    if (m_isBusy) {
        return;
    }

    const QVector<CleanupCategory> selected = collectSelectedCategories();
    qint64 selectedSize = 0;
    int selectedCount = 0;
    for (const CleanupCategory &category : selected) {
        if (!category.selected) {
            continue;
        }
        ++selectedCount;
        if (category.scanned) {
            selectedSize += category.sizeBytes;
        }
    }

    if (selectedCount == 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请至少选择一项清理内容。"));
        return;
    }

    const QString sizeHint = selectedSize > 0
                                 ? CleanupScanner::formatSize(selectedSize)
                                 : QStringLiteral("未知（建议先扫描）");

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("确认清理"),
        QStringLiteral("确定要清理选中的 %1 项吗？\n预计释放空间: %2\n\n此操作不可撤销。")
            .arg(selectedCount)
            .arg(sizeHint));

    if (reply != QMessageBox::Yes) {
        return;
    }

    m_activeCleanupContext = CleanupContext::Disk;
    m_worker->setCategories(selected);
    m_worker->setMode(CleanupWorker::Mode::Clean);
    setUiBusy(true);
    m_statusLabel->setText(QStringLiteral("正在清理，请稍候..."));

    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    m_workerThread->start();
}

void MainWindow::onSelectAllClicked()
{
    for (int i = 0; i < m_categoryTable->rowCount(); ++i) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_categoryTable->cellWidget(i, 0));
        if (checkBox) {
            checkBox->setChecked(true);
        }
    }
    updateSummary();
}

void MainWindow::onSelectNoneClicked()
{
    for (int i = 0; i < m_categoryTable->rowCount(); ++i) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_categoryTable->cellWidget(i, 0));
        if (checkBox) {
            checkBox->setChecked(false);
        }
    }
    updateSummary();
}

void MainWindow::onSoftwareScanClicked()
{
    if (m_isBusy) {
        return;
    }

    m_activeCleanupContext = CleanupContext::Software;
    m_totalScannedBytes = 0;
    for (CleanupCategory &category : m_categories) {
        if (category.group == QStringLiteral("软件")) {
            category.scanned = false;
            category.sizeBytes = 0;
        }
    }
    for (int i = 0; i < m_softwareCategoryIndices.size(); ++i) {
        updateSoftwareCacheRow(i, m_categories.at(m_softwareCategoryIndices.at(i)));
    }

    CleanupScanner::refreshDynamicCategoryPaths(m_categories);
    for (int i = 0; i < m_softwareCategoryIndices.size(); ++i) {
        updateSoftwareCacheRow(i, m_categories.at(m_softwareCategoryIndices.at(i)));
    }

    const QVector<CleanupCategory> payload = collectSoftwareCategoriesForWorker();
    m_worker->setCategories(payload);
    m_worker->setMode(CleanupWorker::Mode::Scan);
    setUiBusy(true);
    m_softwareStatusLabel->setText(QStringLiteral("正在扫描软件缓存，请稍候..."));

    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    m_workerThread->start();
}

void MainWindow::onSoftwareCleanClicked()
{
    if (m_isBusy) {
        return;
    }

    const QVector<CleanupCategory> payload = collectSoftwareCategoriesForWorker();
    qint64 selectedSize = 0;
    int selectedCount = 0;
    for (const CleanupCategory &category : payload) {
        if (!category.selected) {
            continue;
        }
        ++selectedCount;
        if (category.scanned) {
            selectedSize += category.sizeBytes;
        }
    }

    if (selectedCount == 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请至少选择一项软件缓存。"));
        return;
    }

    const QString sizeHint = selectedSize > 0
                                 ? CleanupScanner::formatSize(selectedSize)
                                 : QStringLiteral("未知（建议先扫描）");

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("确认清理"),
        QStringLiteral("确定要清理选中的 %1 项软件缓存吗？\n预计释放空间: %2\n\n清理前请关闭相关软件，此操作不可撤销。")
            .arg(selectedCount)
            .arg(sizeHint));

    if (reply != QMessageBox::Yes) {
        return;
    }

    m_activeCleanupContext = CleanupContext::Software;
    m_worker->setCategories(payload);
    m_worker->setMode(CleanupWorker::Mode::Clean);
    setUiBusy(true);
    m_softwareStatusLabel->setText(QStringLiteral("正在清理软件缓存，请稍候..."));

    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    m_workerThread->start();
}

void MainWindow::onSoftwareSelectAllClicked()
{
    for (int i = 0; i < m_softwareCacheTable->rowCount(); ++i) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_softwareCacheTable->cellWidget(i, 0));
        if (checkBox) {
            checkBox->setChecked(true);
        }
    }
    syncSoftwareSelectionToDiskTable();
    updateSoftwareSummary();
}

void MainWindow::onSoftwareSelectNoneClicked()
{
    for (int i = 0; i < m_softwareCacheTable->rowCount(); ++i) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(m_softwareCacheTable->cellWidget(i, 0));
        if (checkBox) {
            checkBox->setChecked(false);
        }
    }
    syncSoftwareSelectionToDiskTable();
    updateSoftwareSummary();
}

void MainWindow::onCategoryProgress(int index, const CleanupCategory &category)
{
    updateCategoryViews(index, category);
    if (category.scanned) {
        m_totalScannedBytes += category.sizeBytes;
    }
    updateSummary();
    updateSoftwareSummary();

    if (m_activeCleanupContext == CleanupContext::Software) {
        m_softwareStatusLabel->setText(
            QStringLiteral("正在处理: %1 (%2)")
                .arg(category.name, CleanupScanner::formatSize(category.sizeBytes)));
    } else {
        m_statusLabel->setText(
            QStringLiteral("正在处理: %1 (%2)")
                .arg(category.name, CleanupScanner::formatSize(category.sizeBytes)));
    }
}

void MainWindow::onWorkerFinished(bool success, qint64 /*totalBytes*/, const QString &message)
{
    setUiBusy(false);
    if (m_activeCleanupContext == CleanupContext::Software) {
        m_softwareStatusLabel->setText(message);
        syncSoftwareSelectionToDiskTable();
    } else {
        m_statusLabel->setText(message);
    }
    refreshDiskInfo();
    updateSummary();
    updateSoftwareSummary();

    if (success) {
        QMessageBox::information(this, QStringLiteral("完成"), message);
    } else if (!message.contains(QStringLiteral("取消"))) {
        QMessageBox::warning(this, QStringLiteral("提示"), message);
    }
}

void MainWindow::onWorkerError(const QString &message)
{
    if (m_activeCleanupContext == CleanupContext::Software) {
        m_softwareStatusLabel->setText(message);
    } else {
        m_statusLabel->setText(message);
    }
}

void MainWindow::onRefreshAppsClicked()
{
    if (m_isUninstallLoading) {
        return;
    }

    m_installedApps.clear();
    m_appModel->clearApps();
    m_uninstallSearchEdit->clear();
    m_appsLoadStarted = true;
    m_appsLoadFinished = false;

    setUninstallLoading(true);
    m_uninstallStatusLabel->setText(QStringLiteral("正在加载软件列表，请稍候..."));

    m_uninstallWorker->setMode(AppUninstallWorker::Mode::Scan);

    if (m_uninstallWorkerThread->isRunning()) {
        m_uninstallWorkerThread->quit();
        m_uninstallWorkerThread->wait(3000);
    }
    m_uninstallWorkerThread->start();
}

void MainWindow::onUninstallSearchChanged(const QString &text)
{
    Q_UNUSED(text)
    rebuildFilteredApps();
}

void MainWindow::onUninstallClicked()
{
    if (m_isUninstallLoading) {
        return;
    }

    const InstalledApp app = AppUninstaller::loadFullAppDetails(currentSelectedApp());
    if (app.displayName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择要卸载的软件。"));
        return;
    }

    if (!app.canUninstall()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("该软件没有可用的卸载信息。"));
        return;
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("确认卸载"),
        QStringLiteral("确定要卸载以下软件吗？\n\n%1\n版本: %2\n\n将调用官方卸载程序。")
            .arg(app.displayName, app.displayVersion.isEmpty() ? QStringLiteral("未知") : app.displayVersion));

    if (reply != QMessageBox::Yes) {
        return;
    }

    setUninstallLoading(true);
    m_uninstallStatusLabel->setText(QStringLiteral("正在卸载 %1 ...").arg(app.displayName));
    m_uninstallWorker->setMode(AppUninstallWorker::Mode::Uninstall);
    m_uninstallWorker->setTargetApp(app);
    m_uninstallWorker->setForceUninstall(false);

    if (m_uninstallWorkerThread->isRunning()) {
        m_uninstallWorkerThread->quit();
        m_uninstallWorkerThread->wait();
    }
    m_uninstallWorkerThread->start();
}

void MainWindow::onForceUninstallClicked()
{
    if (m_isUninstallLoading) {
        return;
    }

    const InstalledApp app = AppUninstaller::loadFullAppDetails(currentSelectedApp());
    if (app.displayName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择要卸载的软件。"));
        return;
    }

    const QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        QStringLiteral("确认强制卸载"),
        QStringLiteral("强制卸载将尝试：\n1. 结束相关进程\n2. 运行卸载程序\n3. 删除安装目录\n4. 清理注册表项\n\n目标软件：%1\n\n此操作不可撤销，是否继续？")
            .arg(app.displayName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    setUninstallLoading(true);
    m_uninstallStatusLabel->setText(QStringLiteral("正在强制卸载 %1 ...").arg(app.displayName));
    m_uninstallWorker->setMode(AppUninstallWorker::Mode::Uninstall);
    m_uninstallWorker->setTargetApp(app);
    m_uninstallWorker->setForceUninstall(true);

    if (m_uninstallWorkerThread->isRunning()) {
        m_uninstallWorkerThread->quit();
        m_uninstallWorkerThread->wait();
    }
    m_uninstallWorkerThread->start();
}

void MainWindow::onAppScanBatchReady(const QVector<InstalledApp> &apps, int totalCount)
{
    m_installedApps.append(apps);

    const QString keyword = m_uninstallSearchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        m_appTableView->setSortingEnabled(false);
        m_appModel->appendApps(apps);
    } else {
        rebuildFilteredApps();
    }

    m_uninstallStatusLabel->setText(
        QStringLiteral("正在加载软件列表，已发现 %1 款...").arg(totalCount));
}

void MainWindow::onAppTableClicked(const QModelIndex &index)
{
    if (!index.isValid() || index.column() != 4) {
        return;
    }

    const QString path = index.data(Qt::DisplayRole).toString().trimmed();
    if (path.isEmpty()) {
        return;
    }

    QFileInfo info(path);
    QString openPath = path;
    if (!info.exists()) {
        const QDir parentDir = info.dir();
        if (parentDir.exists()) {
            openPath = parentDir.absolutePath();
        } else {
            QMessageBox::warning(this, QStringLiteral("提示"),
                                 QStringLiteral("路径不存在：\n%1").arg(path));
            return;
        }
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(openPath))) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("无法打开路径：\n%1").arg(openPath));
    }
}

void MainWindow::onAppScanFinished(const QVector<InstalledApp> &apps)
{
    setUninstallLoading(false);
    m_appsLoadFinished = true;
    m_installedApps = apps;
    rebuildFilteredApps();
    m_appTableView->setSortingEnabled(true);
    m_appTableView->resizeColumnsToContents();
    m_appTableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_appTableView->resizeRowsToContents();

    if (apps.isEmpty()) {
        m_uninstallStatusLabel->setText(QStringLiteral("未发现可卸载软件，请尝试以管理员身份运行后刷新。"));
    } else {
        m_uninstallStatusLabel->setText(
            QStringLiteral("加载完成，共 %1 款软件（含商店应用），当前显示 %2 款。")
                .arg(apps.size())
                .arg(m_appModel->rowCount()));
    }
}

void MainWindow::onAppUninstallFinished(bool success, const QString &message)
{
    setUninstallLoading(false);
    m_uninstallStatusLabel->setText(message);
    refreshDiskInfo();

    if (success) {
        QMessageBox::information(this, QStringLiteral("完成"), message);
        m_appsLoadFinished = false;
        m_appsLoadStarted = false;
        onRefreshAppsClicked();
    } else {
        QMessageBox::warning(this, QStringLiteral("卸载失败"), message);
    }
}
