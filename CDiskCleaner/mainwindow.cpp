#include "mainwindow.h"

#include "applistmodel.h"
#include "appuninstaller.h"
#include "appuninstallworker.h"
#include "cleanupscanner.h"
#include "cleanupworker.h"

#include <QCheckBox>
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
{
    m_categories = CleanupScanner::defaultCategories();
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
    setWindowTitle(QStringLiteral("C盘清理工具 v1.2.3"));
    resize(1020, 720);

    m_tabWidget = new QTabWidget(this);
    QWidget *cleanupTab = new QWidget(this);
    QWidget *uninstallTab = new QWidget(this);

    setupCleanupTab(cleanupTab);
    setupUninstallTab(uninstallTab);

    m_tabWidget->addTab(cleanupTab, QStringLiteral("磁盘清理"));
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
    m_appTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_appTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_appTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_appTableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_appTableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_appTableView->setSortingEnabled(true);

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
    connect(m_uninstallWorker, &AppUninstallWorker::scanBatchReady, this, &MainWindow::onAppScanBatchReady);
    connect(m_uninstallWorker, &AppUninstallWorker::scanFinished, this, &MainWindow::onAppScanFinished);
    connect(m_uninstallWorker, &AppUninstallWorker::uninstallFinished, this, &MainWindow::onAppUninstallFinished);
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
    m_refreshDiskButton->setEnabled(!busy);
    m_categoryTable->setEnabled(!busy);
    m_progressBar->setVisible(busy);
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
    if (index == 1) {
        startLoadAppsIfNeeded();
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

void MainWindow::onCategoryProgress(int index, const CleanupCategory &category)
{
    if (index >= 0 && index < m_categories.size()) {
        m_categories[index] = category;
        updateCategoryRow(index, category);
        if (category.scanned) {
            m_totalScannedBytes += category.sizeBytes;
        }
    }
    updateSummary();
}

void MainWindow::onWorkerFinished(bool success, qint64 /*totalBytes*/, const QString &message)
{
    setUiBusy(false);
    m_statusLabel->setText(message);
    refreshDiskInfo();
    updateSummary();

    if (success) {
        QMessageBox::information(this, QStringLiteral("完成"), message);
    } else if (!message.contains(QStringLiteral("取消"))) {
        QMessageBox::warning(this, QStringLiteral("提示"), message);
    }
}

void MainWindow::onWorkerError(const QString &message)
{
    m_statusLabel->setText(message);
}

void MainWindow::onRefreshAppsClicked()
{
    if (m_isUninstallLoading) {
        return;
    }

    m_installedApps.clear();
    m_appModel->clearApps();
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

void MainWindow::onAppScanFinished(int totalCount)
{
    setUninstallLoading(false);
    m_appsLoadFinished = true;

    if (m_installedApps.isEmpty() && totalCount > 0) {
        rebuildFilteredApps();
    } else if (m_appModel->rowCount() == 0 && !m_installedApps.isEmpty()) {
        rebuildFilteredApps();
    }

    m_appTableView->setSortingEnabled(true);

    if (totalCount == 0) {
        m_uninstallStatusLabel->setText(QStringLiteral("未发现可卸载软件，请尝试以管理员身份运行后刷新。"));
    } else {
        m_uninstallStatusLabel->setText(
            QStringLiteral("加载完成，共 %1 款软件。").arg(totalCount));
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
