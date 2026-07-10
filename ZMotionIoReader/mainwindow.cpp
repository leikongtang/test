#include "mainwindow.h"
#include "iopanelwidget.h"
#include "serialportenumerator.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
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
#include <QtConcurrent>

namespace {
const char kSettingsOrg[] = "LktCodes";
const char kSettingsApp[] = "ZMotionIoReader";
const char kKeyConnMode[] = "connection/mode";
const char kKeyIp[] = "controller/ip";
const char kKeyComPort[] = "serial/port";
const char kKeyBaud[] = "serial/baud";
const char kKeyParity[] = "serial/parity";
const char kKeyDataBits[] = "serial/dataBits";
const char kKeyStopBits[] = "serial/stopBits";
const char kSettingsIoCount[] = "controller/ioCount";
const char kKeyRefreshMs[] = "controller/refreshMs";
const char kKeyAutoOutput[] = "conversion/autoOutput";
const char kKeyConversionMode[] = "conversion/mode";
const char kKeyOffset[] = "conversion/offset";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_inputHandle(nullptr)
    , m_outputHandle(nullptr)
    , m_pollTimer(new QTimer(this))
    , m_connectWatcher(new QFutureWatcher<ZMotionDualConnectResult>(this))
    , m_connModeCombo(nullptr)
    , m_inputSection(nullptr)
    , m_outputSection(nullptr)
    , m_ipEdit(nullptr)
    , m_comCombo(nullptr)
    , m_refreshPortButton(nullptr)
    , m_baudCombo(nullptr)
    , m_parityCombo(nullptr)
    , m_dataBitsCombo(nullptr)
    , m_stopBitsCombo(nullptr)
    , m_ioCountSpin(nullptr)
    , m_refreshSpin(nullptr)
    , m_connectButton(nullptr)
    , m_disconnectButton(nullptr)
    , m_refreshButton(nullptr)
    , m_statusLabel(nullptr)
    , m_serialLed(nullptr)
    , m_logEdit(nullptr)
    , m_inputPanel(nullptr)
    , m_convertedPanel(nullptr)
    , m_outputPanel(nullptr)
    , m_autoOutputCheck(nullptr)
    , m_modeCombo(nullptr)
    , m_offsetSpin(nullptr)
    , m_conversionDescLabel(nullptr)
    , m_connecting(false)
    , m_ignoreConnectResult(false)
    , m_connectCancelled(0)
    , m_connectToken(0)
    , m_pendingConnectMode(ConnectMode::EthInSerialOut)
    , m_ioCount(32)
    , m_conversionMode(ConversionMode::Direct)
    , m_offset(0)
    , m_lastInputState(0)
    , m_lastConvertedState(0)
    , m_lastOutputState(0)
{
    setupUi();
    loadSettings();
    updateConversionUi();
    updateConnectionUi();
    refreshSerialPortList();

    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onPollTimeout);
    connect(m_connectWatcher, &QFutureWatcher<ZMotionDualConnectResult>::finished,
            this, &MainWindow::onConnectFinished);
    m_pollTimer->setInterval(m_refreshSpin->value());
    setConnected(false);

    appendLog(QStringLiteral("程序已启动，默认模式：网口输入 + 串口输出"));
}

MainWindow::~MainWindow()
{
    waitConnectFinished();
    closeAllHandles();
}

void MainWindow::setupUi()
{
    const QString version = QApplication::applicationVersion();
    setWindowTitle(QStringLiteral("正运动 IO 信号收发转换 v%1").arg(version));
    resize(780, 860);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);

    QGroupBox *connGroup = new QGroupBox(QStringLiteral("控制器连接"), central);
    QGridLayout *connLayout = new QGridLayout(connGroup);

    connLayout->addWidget(new QLabel(QStringLiteral("连接模式："), connGroup), 0, 0);
    m_connModeCombo = new QComboBox(connGroup);
    m_connModeCombo->addItem(connectModeName(ConnectMode::EthInSerialOut),
                             static_cast<int>(ConnectMode::EthInSerialOut));
    m_connModeCombo->addItem(connectModeName(ConnectMode::Ethernet),
                             static_cast<int>(ConnectMode::Ethernet));
    m_connModeCombo->addItem(connectModeName(ConnectMode::Serial),
                             static_cast<int>(ConnectMode::Serial));
    connLayout->addWidget(m_connModeCombo, 0, 1, 1, 2);

    m_inputSection = new QWidget(connGroup);
    QHBoxLayout *inputLayout = new QHBoxLayout(m_inputSection);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->addWidget(new QLabel(QStringLiteral("输入 IP："), m_inputSection));
    m_ipEdit = new QLineEdit(QStringLiteral("192.168.0.11"), m_inputSection);
    m_ipEdit->setPlaceholderText(QStringLiteral("例如 192.168.0.11"));
    inputLayout->addWidget(m_ipEdit, 1);
    connLayout->addWidget(m_inputSection, 1, 0, 1, 3);

    m_outputSection = new QWidget(connGroup);
    QGridLayout *serialLayout = new QGridLayout(m_outputSection);
    serialLayout->setContentsMargins(0, 0, 0, 0);
    serialLayout->addWidget(new QLabel(QStringLiteral("输出串口："), m_outputSection), 0, 0);
    m_comCombo = new QComboBox(m_outputSection);
    m_refreshPortButton = new QPushButton(QStringLiteral("刷新"), m_outputSection);
    QHBoxLayout *comRow = new QHBoxLayout();
    comRow->addWidget(m_comCombo, 1);
    comRow->addWidget(m_refreshPortButton);
    serialLayout->addLayout(comRow, 0, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("波特率："), m_outputSection), 1, 0);
    m_baudCombo = new QComboBox(m_outputSection);
    m_baudCombo->addItem(QStringLiteral("115200"), 115200);
    m_baudCombo->addItem(QStringLiteral("57600"), 57600);
    m_baudCombo->addItem(QStringLiteral("38400"), 38400);
    m_baudCombo->addItem(QStringLiteral("19200"), 19200);
    m_baudCombo->addItem(QStringLiteral("9600"), 9600);
    serialLayout->addWidget(m_baudCombo, 1, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("数据位："), m_outputSection), 2, 0);
    m_dataBitsCombo = new QComboBox(m_outputSection);
    m_dataBitsCombo->addItem(QStringLiteral("8"), 8);
    m_dataBitsCombo->addItem(QStringLiteral("7"), 7);
    serialLayout->addWidget(m_dataBitsCombo, 2, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("校验位："), m_outputSection), 3, 0);
    m_parityCombo = new QComboBox(m_outputSection);
    m_parityCombo->addItem(QStringLiteral("无"), 0);
    m_parityCombo->addItem(QStringLiteral("奇校验"), 1);
    m_parityCombo->addItem(QStringLiteral("偶校验"), 2);
    serialLayout->addWidget(m_parityCombo, 3, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("停止位："), m_outputSection), 4, 0);
    m_stopBitsCombo = new QComboBox(m_outputSection);
    m_stopBitsCombo->addItem(QStringLiteral("1"), 1);
    m_stopBitsCombo->addItem(QStringLiteral("2"), 2);
    serialLayout->addWidget(m_stopBitsCombo, 4, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("虚拟串口连接："), m_outputSection), 5, 0);
    m_serialLed = new ConnectionLedWidget(m_outputSection);
    serialLayout->addWidget(m_serialLed, 5, 1);

    connLayout->addWidget(m_outputSection, 2, 0, 1, 3);

    connLayout->addWidget(new QLabel(QStringLiteral("IO 通道数："), connGroup), 3, 0);
    m_ioCountSpin = new QSpinBox(connGroup);
    m_ioCountSpin->setRange(8, 64);
    m_ioCountSpin->setSingleStep(8);
    m_ioCountSpin->setValue(32);
    connLayout->addWidget(m_ioCountSpin, 3, 1);

    connLayout->addWidget(new QLabel(QStringLiteral("刷新间隔 (ms)："), connGroup), 4, 0);
    m_refreshSpin = new QSpinBox(connGroup);
    m_refreshSpin->setRange(50, 2000);
    m_refreshSpin->setSingleStep(50);
    m_refreshSpin->setValue(100);
    connLayout->addWidget(m_refreshSpin, 4, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton(QStringLiteral("连接"), connGroup);
    m_disconnectButton = new QPushButton(QStringLiteral("断开"), connGroup);
    m_refreshButton = new QPushButton(QStringLiteral("立即刷新"), connGroup);
    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addStretch();
    connLayout->addLayout(buttonLayout, 5, 0, 1, 3);

    m_statusLabel = new QLabel(QStringLiteral("状态：未连接"), connGroup);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
    connLayout->addWidget(m_statusLabel, 6, 0, 1, 3);

    QGroupBox *convertGroup = new QGroupBox(QStringLiteral("信号转换输出"), central);
    QGridLayout *convertLayout = new QGridLayout(convertGroup);

    m_autoOutputCheck = new QCheckBox(QStringLiteral("启用自动转换输出"), convertGroup);
    convertLayout->addWidget(m_autoOutputCheck, 0, 0, 1, 3);

    convertLayout->addWidget(new QLabel(QStringLiteral("转换模式："), convertGroup), 1, 0);
    m_modeCombo = new QComboBox(convertGroup);
    m_modeCombo->addItem(SignalConverter::modeName(ConversionMode::Direct),
                         static_cast<int>(ConversionMode::Direct));
    m_modeCombo->addItem(SignalConverter::modeName(ConversionMode::Invert),
                         static_cast<int>(ConversionMode::Invert));
    m_modeCombo->addItem(SignalConverter::modeName(ConversionMode::Offset),
                         static_cast<int>(ConversionMode::Offset));
    convertLayout->addWidget(m_modeCombo, 1, 1);

    convertLayout->addWidget(new QLabel(QStringLiteral("偏移通道："), convertGroup), 2, 0);
    m_offsetSpin = new QSpinBox(convertGroup);
    m_offsetSpin->setRange(0, 31);
    m_offsetSpin->setValue(0);
    convertLayout->addWidget(m_offsetSpin, 2, 1);

    m_conversionDescLabel = new QLabel(convertGroup);
    m_conversionDescLabel->setWordWrap(true);
    m_conversionDescLabel->setStyleSheet(QStringLiteral("color: #475569;"));
    convertLayout->addWidget(m_conversionDescLabel, 3, 0, 1, 3);

    m_inputPanel = new IoPanelWidget(QStringLiteral("数字输入 IN（接收）"), 32, central);
    m_convertedPanel = new IoPanelWidget(QStringLiteral("转换后输出（预览）"), 32, central);
    m_outputPanel = new IoPanelWidget(QStringLiteral("IO 板输出 OUT（实际）"), 32, central);

    QGroupBox *logGroup = new QGroupBox(QStringLiteral("日志"), central);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QPlainTextEdit(logGroup);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);
    m_logEdit->setFixedHeight(100);
    logLayout->addWidget(m_logEdit);

    mainLayout->addWidget(connGroup);
    mainLayout->addWidget(convertGroup);
    mainLayout->addWidget(m_inputPanel, 1);
    mainLayout->addWidget(m_convertedPanel, 1);
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
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onConversionModeChanged);
    connect(m_offsetSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        m_offset = value;
        updateConversionUi();
    });
    connect(m_autoOutputCheck, &QCheckBox::toggled, this, &MainWindow::onAutoOutputToggled);
    connect(m_connModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onConnectionModeChanged);
    connect(m_refreshPortButton, &QPushButton::clicked, this, &MainWindow::onRefreshPortsClicked);
}

void MainWindow::loadSettings()
{
    QSettings settings(kSettingsOrg, kSettingsApp);

    const int connMode = settings.value(kKeyConnMode, static_cast<int>(ConnectMode::EthInSerialOut)).toInt();
    const int connModeIndex = m_connModeCombo->findData(connMode);
    if (connModeIndex >= 0) {
        m_connModeCombo->setCurrentIndex(connModeIndex);
    }

    m_ipEdit->setText(settings.value(kKeyIp, QStringLiteral("192.168.0.11")).toString());

    const int baud = settings.value(kKeyBaud, 115200).toInt();
    const int baudIndex = m_baudCombo->findData(baud);
    if (baudIndex >= 0) {
        m_baudCombo->setCurrentIndex(baudIndex);
    }

    const int parity = settings.value(kKeyParity, 0).toInt();
    const int parityIndex = m_parityCombo->findData(parity);
    if (parityIndex >= 0) {
        m_parityCombo->setCurrentIndex(parityIndex);
    }

    const int dataBits = settings.value(kKeyDataBits, 8).toInt();
    const int dataBitsIndex = m_dataBitsCombo->findData(dataBits);
    if (dataBitsIndex >= 0) {
        m_dataBitsCombo->setCurrentIndex(dataBitsIndex);
    }

    const int stopBits = settings.value(kKeyStopBits, 1).toInt();
    const int stopBitsIndex = m_stopBitsCombo->findData(stopBits);
    if (stopBitsIndex >= 0) {
        m_stopBitsCombo->setCurrentIndex(stopBitsIndex);
    }

    m_savedComPort = settings.value(kKeyComPort).toString();

    m_ioCountSpin->setValue(settings.value(kSettingsIoCount, 32).toInt());
    m_refreshSpin->setValue(settings.value(kKeyRefreshMs, 100).toInt());
    m_autoOutputCheck->setChecked(settings.value(kKeyAutoOutput, false).toBool());

    const int mode = settings.value(kKeyConversionMode, static_cast<int>(ConversionMode::Direct)).toInt();
    const int modeIndex = m_modeCombo->findData(mode);
    if (modeIndex >= 0) {
        m_modeCombo->setCurrentIndex(modeIndex);
    }
    m_offsetSpin->setValue(settings.value(kKeyOffset, 0).toInt());

    m_ioCount = m_ioCountSpin->value();
    m_conversionMode = static_cast<ConversionMode>(m_modeCombo->currentData().toInt());
    m_offset = m_offsetSpin->value();
}

void MainWindow::saveSettings()
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    settings.setValue(kKeyConnMode, m_connModeCombo->currentData().toInt());
    settings.setValue(kKeyIp, m_ipEdit->text().trimmed());
    settings.setValue(kKeyComPort, m_comCombo->currentData().toString());
    settings.setValue(kKeyBaud, m_baudCombo->currentData().toInt());
    settings.setValue(kKeyParity, m_parityCombo->currentData().toInt());
    settings.setValue(kKeyDataBits, m_dataBitsCombo->currentData().toInt());
    settings.setValue(kKeyStopBits, m_stopBitsCombo->currentData().toInt());
    settings.setValue(kSettingsIoCount, m_ioCountSpin->value());
    settings.setValue(kKeyRefreshMs, m_refreshSpin->value());
    settings.setValue(kKeyAutoOutput, m_autoOutputCheck->isChecked());
    settings.setValue(kKeyConversionMode, m_modeCombo->currentData().toInt());
    settings.setValue(kKeyOffset, m_offsetSpin->value());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    waitConnectFinished();
    closeAllHandles();
    QMainWindow::closeEvent(event);
}

void MainWindow::waitConnectFinished()
{
    if (m_connectWatcher->isRunning()) {
        m_connectWatcher->waitForFinished();
    }
}

void MainWindow::closeAllHandles()
{
    if (m_inputHandle && m_inputHandle == m_outputHandle) {
        if (m_inputHandle) {
            ZAux_Close(m_inputHandle);
        }
    } else {
        if (m_inputHandle) {
            ZAux_Close(m_inputHandle);
        }
        if (m_outputHandle) {
            ZAux_Close(m_outputHandle);
        }
    }
    m_inputHandle = nullptr;
    m_outputHandle = nullptr;
    updateSerialLedState();
}

bool MainWindow::isConnected() const
{
    return m_inputHandle != nullptr && m_outputHandle != nullptr;
}

void MainWindow::setConnectionInputsEnabled(bool enabled)
{
    const ConnectMode mode = currentConnectMode();
    const bool inputEnabled = enabled && (mode == ConnectMode::EthInSerialOut || mode == ConnectMode::Ethernet);
    const bool outputEnabled = enabled && (mode == ConnectMode::EthInSerialOut || mode == ConnectMode::Serial);

    m_connModeCombo->setEnabled(enabled);
    m_ipEdit->setEnabled(inputEnabled);
    m_comCombo->setEnabled(outputEnabled);
    m_refreshPortButton->setEnabled(outputEnabled);
    m_baudCombo->setEnabled(outputEnabled);
    m_dataBitsCombo->setEnabled(outputEnabled);
    m_parityCombo->setEnabled(outputEnabled);
    m_stopBitsCombo->setEnabled(outputEnabled);
    m_ioCountSpin->setEnabled(enabled);
}

void MainWindow::setConnecting(bool connecting, const QString &target)
{
    m_connecting = connecting;
    m_connectButton->setEnabled(!connecting);
    m_refreshButton->setEnabled(!connecting && isConnected());
    setConnectionInputsEnabled(!connecting && !isConnected());
    m_modeCombo->setEnabled(!connecting);
    m_offsetSpin->setEnabled(!connecting && m_conversionMode == ConversionMode::Offset);
    m_autoOutputCheck->setEnabled(!connecting);

    if (connecting) {
        m_disconnectButton->setEnabled(true);
        m_disconnectButton->setText(QStringLiteral("取消连接"));
        m_connectButton->setText(QStringLiteral("连接中..."));
        m_statusLabel->setText(QStringLiteral("状态：正在连接 %1 ...").arg(target));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #d97706; font-weight: bold;"));
    } else {
        m_disconnectButton->setText(QStringLiteral("断开"));
        m_connectButton->setText(QStringLiteral("连接"));
    }

    updateSerialLedState();
}

void MainWindow::cancelConnectingUi()
{
    m_ignoreConnectResult = true;
    m_connecting = false;

    m_connectButton->setEnabled(true);
    m_connectButton->setText(QStringLiteral("连接"));
    m_disconnectButton->setEnabled(false);
    m_disconnectButton->setText(QStringLiteral("断开"));
    m_refreshButton->setEnabled(false);
    setConnectionInputsEnabled(true);
    m_modeCombo->setEnabled(true);
    m_offsetSpin->setEnabled(m_conversionMode == ConversionMode::Offset);
    m_autoOutputCheck->setEnabled(true);

    m_statusLabel->setText(QStringLiteral("状态：连接已取消"));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #64748b; font-weight: bold;"));
    appendLog(QStringLiteral("连接已取消"));
    updateSerialLedState();
}

void MainWindow::setConnected(bool connected)
{
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_disconnectButton->setText(QStringLiteral("断开"));
    m_connectButton->setText(QStringLiteral("连接"));
    m_refreshButton->setEnabled(connected);
    setConnectionInputsEnabled(!connected);
    m_modeCombo->setEnabled(true);
    m_offsetSpin->setEnabled(m_conversionMode == ConversionMode::Offset);
    m_autoOutputCheck->setEnabled(true);
    m_connecting = false;

    if (connected) {
        QString statusText;
        if (m_pendingConnectMode == ConnectMode::EthInSerialOut) {
            statusText = QStringLiteral("状态：已连接 输入:%1 → 输出:%2")
                             .arg(inputAddress(), outputAddress());
        } else if (m_pendingConnectMode == ConnectMode::Ethernet) {
            statusText = QStringLiteral("状态：已连接 %1").arg(inputAddress());
        } else {
            statusText = QStringLiteral("状态：已连接 %1").arg(outputAddress());
        }
        m_statusLabel->setText(statusText);
        m_statusLabel->setStyleSheet(QStringLiteral("color: #15803d; font-weight: bold;"));
        m_pollTimer->start();
    } else {
        m_pollTimer->stop();
        m_statusLabel->setText(QStringLiteral("状态：未连接"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #64748b; font-weight: bold;"));
        m_inputPanel->setAllStates(0);
        m_convertedPanel->setAllStates(0);
        m_outputPanel->setAllStates(0);
        m_lastInputState = 0;
        m_lastConvertedState = 0;
        m_lastOutputState = 0;
        updateConnectionUi();
    }

    updateSerialLedState();
}

void MainWindow::showConnectFailed(const QString &target, int errorCode)
{
    setConnected(false);
    m_statusLabel->setText(QStringLiteral("状态：连接失败（%1）").arg(target));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));

    appendLog(QStringLiteral("连接失败：无法连接到 %1，%2").arg(target, errorText(errorCode)));

    QString hint;
    if (m_pendingConnectMode == ConnectMode::Ethernet
        || (m_pendingConnectMode == ConnectMode::EthInSerialOut && target == inputAddress())) {
        hint = QStringLiteral("请检查：\n"
                              "1. IP 地址是否正确\n"
                              "2. 网线是否已连接\n"
                              "3. PC 与 IO 板是否在同一网段");
    } else {
        hint = QStringLiteral("请检查：\n"
                              "1. 串口是否被其他程序占用\n"
                              "2. 波特率等参数是否正确\n"
                              "3. 虚拟串口配对是否正常（com0com 等）");
    }

    QMessageBox::critical(this,
                          QStringLiteral("连接失败"),
                          QStringLiteral("连接失败，无法连接到 %1。\n\n%2\n\n%3")
                              .arg(target, hint, errorText(errorCode)));
}

ConnectMode MainWindow::currentConnectMode() const
{
    return static_cast<ConnectMode>(m_connModeCombo->currentData().toInt());
}

QString MainWindow::inputAddress() const
{
    return m_ipEdit->text().trimmed();
}

QString MainWindow::outputAddress() const
{
    return m_comCombo->currentData().toString();
}

void MainWindow::updateSerialLedState()
{
    const ConnectMode mode = currentConnectMode();
    const bool usesSerial = mode == ConnectMode::EthInSerialOut || mode == ConnectMode::Serial;
    m_serialLed->setVisibleForSerial(usesSerial);

    if (!usesSerial) {
        m_serialLed->setConnected(false);
        return;
    }

    m_serialLed->setConnected(isConnected() && m_outputHandle != nullptr);
}

void MainWindow::updateConnectionUi()
{
    const ConnectMode mode = currentConnectMode();
    m_inputSection->setVisible(mode == ConnectMode::EthInSerialOut || mode == ConnectMode::Ethernet);
    m_outputSection->setVisible(mode == ConnectMode::EthInSerialOut || mode == ConnectMode::Serial);

    if (mode == ConnectMode::EthInSerialOut) {
        m_inputPanel->setTitle(QStringLiteral("数字输入 IN（网口接收）"));
        m_outputPanel->setTitle(QStringLiteral("数字输出 OUT（串口实际）"));
        m_autoOutputCheck->setText(QStringLiteral("启用自动转换输出到串口"));
    } else if (mode == ConnectMode::Ethernet) {
        m_inputPanel->setTitle(QStringLiteral("数字输入 IN（接收）"));
        m_outputPanel->setTitle(QStringLiteral("数字输出 OUT（实际）"));
        m_autoOutputCheck->setText(QStringLiteral("启用自动转换输出到 IO 板"));
    } else {
        m_inputPanel->setTitle(QStringLiteral("数字输入 IN（接收）"));
        m_outputPanel->setTitle(QStringLiteral("数字输出 OUT（实际）"));
        m_autoOutputCheck->setText(QStringLiteral("启用自动转换输出到 IO 板"));
    }

    updateSerialLedState();
}

void MainWindow::refreshSerialPortList()
{
    const QString currentPort = m_comCombo->currentData().toString();
    m_comCombo->clear();

    const auto ports = listAvailableSerialPorts();
    if (ports.isEmpty()) {
        m_comCombo->addItem(QStringLiteral("未检测到串口"), QString());
        return;
    }

    for (const SerialPortEntry &entry : ports) {
        QString label = entry.portName;
        if (entry.isVirtual) {
            label += QStringLiteral(" [虚拟串口]");
        }
        if (!entry.description.isEmpty()) {
            label += QStringLiteral(" - %1").arg(entry.description);
        }
        m_comCombo->addItem(label, entry.portName);
    }

    const QString restorePort = !m_savedComPort.isEmpty() ? m_savedComPort : currentPort;
    const int index = m_comCombo->findData(restorePort);
    if (index >= 0) {
        m_comCombo->setCurrentIndex(index);
    }
    m_savedComPort.clear();
}

ZMotionDualConnectRequest MainWindow::buildDualConnectRequest()
{
    ZMotionDualConnectRequest request;
    request.mode = currentConnectMode();
    request.inputAddress = inputAddress();
    request.outputAddress = outputAddress();
    request.baudRate = m_baudCombo->currentData().toInt();
    request.dataBits = m_dataBitsCombo->currentData().toInt();
    request.parity = m_parityCombo->currentData().toInt();
    request.stopBits = m_stopBitsCombo->currentData().toInt();
    request.cancelled = &m_connectCancelled;
    request.token = m_connectToken;
    return request;
}

void MainWindow::onConnectionModeChanged(int index)
{
    Q_UNUSED(index)
    updateConnectionUi();
    if (!m_connecting && !isConnected()) {
        setConnected(false);
    }
}

void MainWindow::onRefreshPortsClicked()
{
    refreshSerialPortList();
    appendLog(QStringLiteral("已刷新串口列表，共 %1 个端口").arg(m_comCombo->count()));
}

void MainWindow::updateConversionUi()
{
    const bool isOffset = m_conversionMode == ConversionMode::Offset;
    m_offsetSpin->setEnabled(isOffset);

    QString desc;
    switch (m_conversionMode) {
    case ConversionMode::Direct:
        desc = QStringLiteral("IN(n) → OUT(n)，输入信号原样输出到对应通道。");
        break;
    case ConversionMode::Invert:
        desc = QStringLiteral("IN(n) → OUT(n) 取反，高电平变低电平，低电平变高电平。");
        break;
    case ConversionMode::Offset:
        desc = QStringLiteral("IN(n) → OUT(n+%1)，输入通道偏移映射到输出通道。").arg(m_offset);
        break;
    }
    m_conversionDescLabel->setText(desc);
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
    if (errorCode == kZMotionConnectCancelled) {
        return QStringLiteral("连接已取消");
    }
    return QStringLiteral("错误码 %1").arg(errorCode);
}

uint32_t MainWindow::applyConversion(uint32_t inputMask) const
{
    return SignalConverter::convert(inputMask, m_ioCount, m_conversionMode, m_offset);
}

bool MainWindow::writeOutputState(uint32_t outputMask)
{
    if (!m_outputHandle) {
        return false;
    }

    const int lastChannel = m_ioCount - 1;
    const int32_t outValue = static_cast<int32_t>(outputMask);
    const int rc = ZAux_Direct_SetOutMulti(m_outputHandle, 0, lastChannel, outValue);
    if (rc != ERR_OK) {
        appendLog(QStringLiteral("写入输出失败：%1").arg(errorText(rc)));
        return false;
    }
    return true;
}

void MainWindow::onConnectClicked()
{
    const ConnectMode mode = currentConnectMode();
    if (inputAddress().isEmpty() && (mode == ConnectMode::EthInSerialOut || mode == ConnectMode::Ethernet)) {
        m_statusLabel->setText(QStringLiteral("状态：连接失败"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
        appendLog(QStringLiteral("连接失败：未输入 IP 地址"));
        QMessageBox::critical(this,
                              QStringLiteral("连接失败"),
                              QStringLiteral("连接失败，请输入网口 IP 地址。"));
        return;
    }

    if (outputAddress().isEmpty() && (mode == ConnectMode::EthInSerialOut || mode == ConnectMode::Serial)) {
        m_statusLabel->setText(QStringLiteral("状态：连接失败"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
        appendLog(QStringLiteral("连接失败：未选择输出串口"));
        QMessageBox::critical(this,
                              QStringLiteral("连接失败"),
                              QStringLiteral("连接失败，请选择输出串口。可点击「刷新」重新扫描串口。"));
        return;
    }

    closeAllHandles();

    m_ignoreConnectResult = false;
    m_connectCancelled.storeRelease(0);
    m_pendingConnectMode = mode;
    ++m_connectToken;

    const ZMotionDualConnectRequest request = buildDualConnectRequest();
    QString connectDesc;
    if (mode == ConnectMode::EthInSerialOut) {
        connectDesc = QStringLiteral("输入:%1 → 输出:%2").arg(inputAddress(), outputAddress());
    } else if (mode == ConnectMode::Ethernet) {
        connectDesc = inputAddress();
    } else {
        connectDesc = outputAddress();
    }

    setConnecting(true, connectDesc);
    appendLog(QStringLiteral("正在连接 %1 ...").arg(connectDesc));

    m_connectWatcher->setFuture(QtConcurrent::run(zmotionConnectDual, request));
}

void MainWindow::onConnectFinished()
{
    const ZMotionDualConnectResult result = m_connectWatcher->result();

    auto cleanupResult = [](const ZMotionDualConnectResult &r) {
        if (r.inputHandle && r.inputHandle == r.outputHandle) {
            if (r.inputHandle) {
                ZAux_Close(r.inputHandle);
            }
        } else {
            if (r.inputHandle) {
                ZAux_Close(r.inputHandle);
            }
            if (r.outputHandle) {
                ZAux_Close(r.outputHandle);
            }
        }
    };

    if (result.token != m_connectToken) {
        cleanupResult(result);
        return;
    }

    if (m_ignoreConnectResult || result.errorCode == kZMotionConnectCancelled) {
        cleanupResult(result);
        return;
    }

    setConnecting(false);

    if (result.errorCode != ERR_OK) {
        cleanupResult(result);
        showConnectFailed(result.failedTarget, result.errorCode);
        return;
    }

    m_inputHandle = result.inputHandle;
    m_outputHandle = result.outputHandle;
    m_ioCount = m_ioCountSpin->value();
    appendLog(QStringLiteral("已连接 %1，IO 通道数 %2，转换模式：%3")
                  .arg(connectModeName(m_pendingConnectMode))
                  .arg(m_ioCount)
                  .arg(SignalConverter::modeName(m_conversionMode)));
    setConnected(true);
    pollIoStates();
}

void MainWindow::onDisconnectClicked()
{
    if (m_connecting) {
        m_connectCancelled.storeRelease(1);
        cancelConnectingUi();
        return;
    }

    if (isConnected()) {
        closeAllHandles();
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

void MainWindow::onConversionModeChanged(int index)
{
    Q_UNUSED(index)
    m_conversionMode = static_cast<ConversionMode>(m_modeCombo->currentData().toInt());
    updateConversionUi();
}

void MainWindow::onAutoOutputToggled(bool enabled)
{
    if (enabled) {
        m_lastOutputState = 0xFFFFFFFFU;
        appendLog(QStringLiteral("已启用自动转换输出"));
    } else {
        m_lastOutputState = 0;
        appendLog(QStringLiteral("已关闭自动转换输出，仅预览转换结果"));
    }
}

bool MainWindow::pollIoStates()
{
    if (!m_inputHandle || !m_outputHandle) {
        return false;
    }

    const int lastChannel = m_ioCount - 1;
    int32_t inputState = 0;
    uint32_t outputState = 0;

    int rc = ZAux_Direct_GetInMulti(m_inputHandle, 0, lastChannel, &inputState);
    if (rc != ERR_OK) {
        appendLog(QStringLiteral("读取输入失败：%1").arg(errorText(rc)));
        return false;
    }

    const uint32_t inputMask = (m_ioCount >= 32) ? static_cast<uint32_t>(inputState)
                                                 : static_cast<uint32_t>(inputState) & ((1U << m_ioCount) - 1U);
    const uint32_t convertedMask = applyConversion(inputMask);

    if (inputState != m_lastInputState) {
        m_inputPanel->setAllStates(inputMask);
        m_lastInputState = inputState;
    }

    if (convertedMask != m_lastConvertedState) {
        m_convertedPanel->setAllStates(convertedMask);
        m_lastConvertedState = convertedMask;
    }

    if (m_autoOutputCheck->isChecked()) {
        if (convertedMask != m_lastOutputState) {
            if (writeOutputState(convertedMask)) {
                m_outputPanel->setAllStates(convertedMask);
                m_lastOutputState = convertedMask;
            }
        }
    } else {
        rc = ZAux_Direct_GetOutMulti(m_outputHandle, 0, lastChannel, &outputState);
        if (rc != ERR_OK) {
            appendLog(QStringLiteral("读取输出失败：%1").arg(errorText(rc)));
            return false;
        }

        const uint32_t outputMask = (m_ioCount >= 32) ? outputState
                                                      : outputState & ((1U << m_ioCount) - 1U);
        if (outputMask != m_lastOutputState) {
            m_outputPanel->setAllStates(outputMask);
            m_lastOutputState = outputMask;
        }
    }

    return true;
}
