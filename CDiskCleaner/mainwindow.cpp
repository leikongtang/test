#include "mainwindow.h"

#include "cleanupscanner.h"
#include "cleanupworker.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isBusy(false)
    , m_totalScannedBytes(0)
{
    m_categories = CleanupScanner::defaultCategories();
    setupUi();
    setupWorker();
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
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("C盘清理工具 v1.0.0"));
    resize(860, 620);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *diskLayout = new QHBoxLayout();
    QLabel *diskTitle = new QLabel(QStringLiteral("C: 盘空间"), this);
    diskTitle->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 14px;"));
    m_diskTotalLabel = new QLabel(this);
    m_diskUsedLabel = new QLabel(this);
    m_diskFreeLabel = new QLabel(this);
    m_refreshDiskButton = new QPushButton(QStringLiteral("刷新"), this);
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

    m_categoryTable = new QTableWidget(m_categories.size(), 5, this);
    m_categoryTable->setHorizontalHeaderLabels({
        QStringLiteral("选择"),
        QStringLiteral("清理项目"),
        QStringLiteral("说明"),
        QStringLiteral("路径"),
        QStringLiteral("大小")
    });
    m_categoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_categoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_categoryTable->verticalHeader()->setVisible(false);
    m_categoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_categoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int i = 0; i < m_categories.size(); ++i) {
        updateCategoryRow(i, m_categories.at(i));
    }

    QHBoxLayout *toolLayout = new QHBoxLayout();
    m_scanButton = new QPushButton(QStringLiteral("扫描"), this);
    m_cleanButton = new QPushButton(QStringLiteral("清理选中项"), this);
    m_selectAllButton = new QPushButton(QStringLiteral("全选"), this);
    m_selectNoneButton = new QPushButton(QStringLiteral("全不选"), this);
    m_scanResultLabel = new QLabel(QStringLiteral("可清理空间: --"), this);
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

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);

    m_statusLabel = new QLabel(QStringLiteral("就绪。建议先扫描，确认后再清理。"), this);

    QLabel *tipLabel = new QLabel(
        QStringLiteral("提示：带「需管理员」标记的项目请以管理员身份运行本程序后再清理。"), this);
    tipLabel->setStyleSheet(QStringLiteral("color: #666;"));

    mainLayout->addLayout(diskLayout);
    mainLayout->addWidget(m_categoryTable, 1);
    mainLayout->addLayout(toolLayout);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(tipLabel);

    setCentralWidget(centralWidget);
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

    QString nameText = category.name;
    if (category.requiresAdmin) {
        nameText += QStringLiteral(" (需管理员)");
    }

    m_categoryTable->setItem(row, 1, new QTableWidgetItem(nameText));
    m_categoryTable->setItem(row, 2, new QTableWidgetItem(category.description));
    m_categoryTable->setItem(row, 3, new QTableWidgetItem(category.path.isEmpty()
                                                               ? QStringLiteral("系统回收站")
                                                               : category.path));

    const QString sizeText = category.scanned
                                 ? CleanupScanner::formatSize(category.sizeBytes)
                                 : QStringLiteral("--");
    m_categoryTable->setItem(row, 4, new QTableWidgetItem(sizeText));
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
