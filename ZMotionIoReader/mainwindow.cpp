#include "mainwindow.h"
#include "iopanelwidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

namespace {
const char kSettingsOrg[] = "LktCodes";
const char kSettingsApp[] = "ZMotionIoReader";
const char kKeyIp[] = "controller/ip";
const char kKeyIoCount[] = "controller/ioCount";
const char kKeyRefreshMs[] = "controller/refreshMs";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_handle(nullptr)
    , m_pollTimer(new QTimer(this))
    , m_ipEdit(nullptr)
    , m_ioCountSpin(nullptr)
    , m_refreshSpin(nullptr)
    , m_connectButton(nullptr)
    , m_disconnectButton(nullptr)
    , m_refreshButton(nullptr)
    , m_statusLabel(nullptr)
    , m_logEdit(nullptr)
    , m_inputPanel(nullptr)
    , m_outputPanel(nullptr)
    , m_ioCount(32)
    , m_lastInputState(0)
    , m_lastOutputState(0)
{
    setupUi();
    loadSettings();

    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onPollTimeout);
    m_pollTimer->setInterval(m_refreshSpin->value());
    setConnected(false);

    appendLog(QStringLiteral("程序已启动，默认正运动 IO 板地址：192.168.0.11"));
}

MainWindow::~MainWindow()
{
    if (m_handle) {
        ZAux_Close(m_handle);
        m_handle = nullptr;
    }
}

void MainWindow::setupUi()
{
    const QString version = QApplication::applicationVersion();
    setWindowTitle(QStringLiteral("正运动 IO 板信号读取 v%1").arg(version));
    resize(760, 720);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);

    QGroupBox *connGroup = new QGroupBox(QStringLiteral("控制器连接"), central);
    QGridLayout *connLayout = new QGridLayout(connGroup);

    connLayout->addWidget(new QLabel(QStringLiteral("IO 板 IP 地址："), connGroup), 0, 0);
    m_ipEdit = new QLineEdit(QStringLiteral("192.168.0.11"), connGroup);
    m_ipEdit->setPlaceholderText(QStringLiteral("例如 192.168.0.11"));
    connLayout->addWidget(m_ipEdit, 0, 1, 1, 2);

    connLayout->addWidget(new QLabel(QStringLiteral("IO 通道数："), connGroup), 1, 0);
    m_ioCountSpin = new QSpinBox(connGroup);
    m_ioCountSpin->setRange(8, 64);
    m_ioCountSpin->setSingleStep(8);
    m_ioCountSpin->setValue(32);
    connLayout->addWidget(m_ioCountSpin, 1, 1);

    connLayout->addWidget(new QLabel(QStringLiteral("刷新间隔 (ms)："), connGroup), 2, 0);
    m_refreshSpin = new QSpinBox(connGroup);
    m_refreshSpin->setRange(50, 2000);
    m_refreshSpin->setSingleStep(50);
    m_refreshSpin->setValue(100);
    connLayout->addWidget(m_refreshSpin, 2, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton(QStringLiteral("连接"), connGroup);
    m_disconnectButton = new QPushButton(QStringLiteral("断开"), connGroup);
    m_refreshButton = new QPushButton(QStringLiteral("立即刷新"), connGroup);
    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addStretch();
    connLayout->addLayout(buttonLayout, 3, 0, 1, 3);

    m_statusLabel = new QLabel(QStringLiteral("状态：未连接"), connGroup);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
    connLayout->addWidget(m_statusLabel, 4, 0, 1, 3);

    m_inputPanel = new IoPanelWidget(QStringLiteral("数字输入 IN"), 32, central);
    m_outputPanel = new IoPanelWidget(QStringLiteral("数字输出 OUT"), 32, central);

    QGroupBox *logGroup = new QGroupBox(QStringLiteral("日志"), central);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QPlainTextEdit(logGroup);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);
    m_logEdit->setFixedHeight(120);
    logLayout->addWidget(m_logEdit);

    mainLayout->addWidget(connGroup);
    mainLayout->addWidget(m_inputPanel, 1);
    mainLayout->addWidget(m_outputPanel, 1);
    mainLayout->addWidget(logGroup);
    setCentralWidget(central);

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_refreshSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onRefreshIntervalChanged);
    connect(m_ioCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onIoCountChanged);
}

void MainWindow::loadSettings()
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    m_ipEdit->setText(settings.value(kKeyIp, QStringLiteral("192.168.0.11")).toString());
    m_ioCountSpin->setValue(settings.value(kKeyIoCount, 32).toInt());
    m_refreshSpin->setValue(settings.value(kKeyRefreshMs, 100).toInt());
    m_ioCount = m_ioCountSpin->value();
}

void MainWindow::saveSettings()
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    settings.setValue(kKeyIp, m_ipEdit->text().trimmed());
    settings.setValue(kKeyIoCount, m_ioCountSpin->value());
    settings.setValue(kKeyRefreshMs, m_refreshSpin->value());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (m_handle) {
        ZAux_Close(m_handle);
        m_handle = nullptr;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setConnected(bool connected)
{
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_refreshButton->setEnabled(connected);
    m_ipEdit->setEnabled(!connected);
    m_ioCountSpin->setEnabled(!connected);

    if (connected) {
        m_statusLabel->setText(QStringLiteral("状态：已连接 %1").arg(m_ipEdit->text().trimmed()));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #15803d; font-weight: bold;"));
        m_pollTimer->start();
    } else {
        m_pollTimer->stop();
        m_statusLabel->setText(QStringLiteral("状态：未连接"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
        m_inputPanel->setAllStates(0);
        m_outputPanel->setAllStates(0);
        m_lastInputState = 0;
        m_lastOutputState = 0;
    }
}

void MainWindow::appendLog(const QString &message)
{
    const QString line = QStringLiteral("[%1] %2")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")))
                             .arg(message);
    m_logEdit->appendPlainText(line);
}

QString MainWindow::errorText(int errorCode) const
{
    return QStringLiteral("错误码 %1").arg(errorCode);
}

void MainWindow::onConnectClicked()
{
    const QString ip = m_ipEdit->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("连接失败"), QStringLiteral("请输入 IO 板 IP 地址。"));
        return;
    }

    if (m_handle) {
        ZAux_Close(m_handle);
        m_handle = nullptr;
    }

    QByteArray ipBytes = ip.toLocal8Bit();
    const int rc = ZAux_OpenEth(ipBytes.data(), &m_handle);
    if (rc != ERR_OK) {
        m_handle = nullptr;
        appendLog(QStringLiteral("连接失败：%1，%2").arg(ip, errorText(rc)));
        QMessageBox::warning(this,
                             QStringLiteral("连接失败"),
                             QStringLiteral("无法连接到 %1\n%2").arg(ip, errorText(rc)));
        setConnected(false);
        return;
    }

    m_ioCount = m_ioCountSpin->value();
    appendLog(QStringLiteral("已连接到 %1，IO 通道数 %2").arg(ip).arg(m_ioCount));
    setConnected(true);
    pollIoStates();
}

void MainWindow::onDisconnectClicked()
{
    if (m_handle) {
        ZAux_Close(m_handle);
        m_handle = nullptr;
        appendLog(QStringLiteral("已断开连接"));
    }
    setConnected(false);
}

void MainWindow::onRefreshClicked()
{
    pollIoStates();
}

void MainWindow::onPollTimeout()
{
    pollIoStates();
}

void MainWindow::onRefreshIntervalChanged(int value)
{
    m_pollTimer->setInterval(value);
}

void MainWindow::onIoCountChanged(int value)
{
    m_ioCount = value;
}

bool MainWindow::pollIoStates()
{
    if (!m_handle) {
        return false;
    }

    const int lastChannel = m_ioCount - 1;
    int32_t inputState = 0;
    uint32_t outputState = 0;

    int rc = ZAux_Direct_GetInMulti(m_handle, 0, lastChannel, &inputState);
    if (rc != ERR_OK) {
        appendLog(QStringLiteral("读取输入失败：%1").arg(errorText(rc)));
        return false;
    }

    rc = ZAux_Direct_GetOutMulti(m_handle, 0, lastChannel, &outputState);
    if (rc != ERR_OK) {
        appendLog(QStringLiteral("读取输出失败：%1").arg(errorText(rc)));
        return false;
    }

    const uint32_t inputMask = (m_ioCount >= 32) ? static_cast<uint32_t>(inputState)
                                                 : static_cast<uint32_t>(inputState) & ((1U << m_ioCount) - 1U);
    const uint32_t outputMask = (m_ioCount >= 32) ? outputState
                                                  : outputState & ((1U << m_ioCount) - 1U);

    if (inputState != m_lastInputState) {
        m_inputPanel->setAllStates(inputMask);
        m_lastInputState = inputState;
    }

    if (outputState != m_lastOutputState) {
        m_outputPanel->setAllStates(outputMask);
        m_lastOutputState = outputState;
    }

    return true;
}
