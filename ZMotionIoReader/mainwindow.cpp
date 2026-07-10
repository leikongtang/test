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
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

namespace {
const char kSettingsOrg[] = "LktCodes";
const char kSettingsApp[] = "ZMotionIoReader";
const char kKeyConnType[] = "connection/type";
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
    , m_handle(nullptr)
    , m_pollTimer(new QTimer(this))
    , m_connectWatcher(new QFutureWatcher<ZMotionConnectResult>(this))
    , m_connTypeCombo(nullptr)
    , m_targetStack(nullptr)
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
    , m_pendingConnectType(ConnectType::Ethernet)
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
    connect(m_connectWatcher, &QFutureWatcher<ZMotionConnectResult>::finished,
            this, &MainWindow::onConnectFinished);
    m_pollTimer->setInterval(m_refreshSpin->value());
    setConnected(false);

    appendLog(QStringLiteral("程序已启动，支持以太网、实际串口和虚拟串口连接"));
}

MainWindow::~MainWindow()
{
    waitConnectFinished();
    if (m_handle) {
        ZAux_Close(m_handle);
        m_handle = nullptr;
    }
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

    connLayout->addWidget(new QLabel(QStringLiteral("连接方式："), connGroup), 0, 0);
    m_connTypeCombo = new QComboBox(connGroup);
    m_connTypeCombo->addItem(connectTypeName(ConnectType::Ethernet),
                             static_cast<int>(ConnectType::Ethernet));
    m_connTypeCombo->addItem(connectTypeName(ConnectType::Serial),
                             static_cast<int>(ConnectType::Serial));
    connLayout->addWidget(m_connTypeCombo, 0, 1, 1, 2);

    m_targetStack = new QStackedWidget(connGroup);
    QWidget *ethPage = new QWidget(connGroup);
    QHBoxLayout *ethLayout = new QHBoxLayout(ethPage);
    ethLayout->setContentsMargins(0, 0, 0, 0);
    ethLayout->addWidget(new QLabel(QStringLiteral("IP 地址："), ethPage));
    m_ipEdit = new QLineEdit(QStringLiteral("192.168.0.11"), ethPage);
    m_ipEdit->setPlaceholderText(QStringLiteral("例如 192.168.0.11"));
    ethLayout->addWidget(m_ipEdit, 1);

    QWidget *serialPage = new QWidget(connGroup);
    QGridLayout *serialLayout = new QGridLayout(serialPage);
    serialLayout->setContentsMargins(0, 0, 0, 0);
    serialLayout->addWidget(new QLabel(QStringLiteral("串口："), serialPage), 0, 0);
    m_comCombo = new QComboBox(serialPage);
    m_refreshPortButton = new QPushButton(QStringLiteral("刷新"), serialPage);
    QHBoxLayout *comRow = new QHBoxLayout();
    comRow->addWidget(m_comCombo, 1);
    comRow->addWidget(m_refreshPortButton);
    serialLayout->addLayout(comRow, 0, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("波特率："), serialPage), 1, 0);
    m_baudCombo = new QComboBox(serialPage);
    m_baudCombo->addItem(QStringLiteral("115200"), 115200);
    m_baudCombo->addItem(QStringLiteral("57600"), 57600);
    m_baudCombo->addItem(QStringLiteral("38400"), 38400);
    m_baudCombo->addItem(QStringLiteral("19200"), 19200);
    m_baudCombo->addItem(QStringLiteral("9600"), 9600);
    serialLayout->addWidget(m_baudCombo, 1, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("数据位："), serialPage), 2, 0);
    m_dataBitsCombo = new QComboBox(serialPage);
    m_dataBitsCombo->addItem(QStringLiteral("8"), 8);
    m_dataBitsCombo->addItem(QStringLiteral("7"), 7);
    serialLayout->addWidget(m_dataBitsCombo, 2, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("校验位："), serialPage), 3, 0);
    m_parityCombo = new QComboBox(serialPage);
    m_parityCombo->addItem(QStringLiteral("无"), 0);
    m_parityCombo->addItem(QStringLiteral("奇校验"), 1);
    m_parityCombo->addItem(QStringLiteral("偶校验"), 2);
    serialLayout->addWidget(m_parityCombo, 3, 1);

    serialLayout->addWidget(new QLabel(QStringLiteral("停止位："), serialPage), 4, 0);
    m_stopBitsCombo = new QComboBox(serialPage);
    m_stopBitsCombo->addItem(QStringLiteral("1"), 1);
    m_stopBitsCombo->addItem(QStringLiteral("2"), 2);
    serialLayout->addWidget(m_stopBitsCombo, 4, 1);

    m_targetStack->addWidget(ethPage);
    m_targetStack->addWidget(serialPage);
    connLayout->addWidget(m_targetStack, 1, 0, 1, 3);

    connLayout->addWidget(new QLabel(QStringLiteral("IO 通道数："), connGroup), 2, 0);
    m_ioCountSpin = new QSpinBox(connGroup);
    m_ioCountSpin->setRange(8, 64);
    m_ioCountSpin->setSingleStep(8);
    m_ioCountSpin->setValue(32);
    connLayout->addWidget(m_ioCountSpin, 2, 1);

    connLayout->addWidget(new QLabel(QStringLiteral("刷新间隔 (ms)："), connGroup), 3, 0);
    m_refreshSpin = new QSpinBox(connGroup);
    m_refreshSpin->setRange(50, 2000);
    m_refreshSpin->setSingleStep(50);
    m_refreshSpin->setValue(100);
    connLayout->addWidget(m_refreshSpin, 3, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton(QStringLiteral("连接"), connGroup);
    m_disconnectButton = new QPushButton(QStringLiteral("断开"), connGroup);
    m_refreshButton = new QPushButton(QStringLiteral("立即刷新"), connGroup);
    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addStretch();
    connLayout->addLayout(buttonLayout, 4, 0, 1, 3);

    m_statusLabel = new QLabel(QStringLiteral("状态：未连接"), connGroup);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
    connLayout->addWidget(m_statusLabel, 5, 0, 1, 3);

    QGroupBox *convertGroup = new QGroupBox(QStringLiteral("信号转换输出"), central);
    QGridLayout *convertLayout = new QGridLayout(convertGroup);

    m_autoOutputCheck = new QCheckBox(QStringLiteral("启用自动转换输出到 IO 板"), convertGroup);
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
    connect(m_connTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onConnectionTypeChanged);
    connect(m_refreshPortButton, &QPushButton::clicked, this, &MainWindow::onRefreshPortsClicked);
}

void MainWindow::loadSettings()
{
    QSettings settings(kSettingsOrg, kSettingsApp);

    const int connType = settings.value(kKeyConnType, static_cast<int>(ConnectType::Ethernet)).toInt();
    const int connTypeIndex = m_connTypeCombo->findData(connType);
    if (connTypeIndex >= 0) {
        m_connTypeCombo->setCurrentIndex(connTypeIndex);
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
    settings.setValue(kKeyConnType, m_connTypeCombo->currentData().toInt());
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
    if (m_handle) {
        ZAux_Close(m_handle);
        m_handle = nullptr;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::waitConnectFinished()
{
    if (m_connectWatcher->isRunning()) {
        m_connectWatcher->waitForFinished();
    }
}

void MainWindow::setConnecting(bool connecting, const QString &target)
{
    m_connecting = connecting;
    m_connectButton->setEnabled(!connecting);
    m_refreshButton->setEnabled(!connecting && m_handle != nullptr);
    m_connTypeCombo->setEnabled(!connecting && m_handle == nullptr);
    m_ipEdit->setEnabled(!connecting && m_handle == nullptr
                         && currentConnectType() == ConnectType::Ethernet);
    m_comCombo->setEnabled(!connecting && m_handle == nullptr
                           && currentConnectType() == ConnectType::Serial);
    m_refreshPortButton->setEnabled(!connecting && m_handle == nullptr
                                    && currentConnectType() == ConnectType::Serial);
    m_baudCombo->setEnabled(!connecting && m_handle == nullptr
                            && currentConnectType() == ConnectType::Serial);
    m_dataBitsCombo->setEnabled(!connecting && m_handle == nullptr
                                && currentConnectType() == ConnectType::Serial);
    m_parityCombo->setEnabled(!connecting && m_handle == nullptr
                              && currentConnectType() == ConnectType::Serial);
    m_stopBitsCombo->setEnabled(!connecting && m_handle == nullptr
                                && currentConnectType() == ConnectType::Serial);
    m_ioCountSpin->setEnabled(!connecting && m_handle == nullptr);
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
    m_connTypeCombo->setEnabled(true);
    m_ipEdit->setEnabled(currentConnectType() == ConnectType::Ethernet);
    m_comCombo->setEnabled(currentConnectType() == ConnectType::Serial);
    m_refreshPortButton->setEnabled(currentConnectType() == ConnectType::Serial);
    m_baudCombo->setEnabled(currentConnectType() == ConnectType::Serial);
    m_dataBitsCombo->setEnabled(currentConnectType() == ConnectType::Serial);
    m_parityCombo->setEnabled(currentConnectType() == ConnectType::Serial);
    m_stopBitsCombo->setEnabled(currentConnectType() == ConnectType::Serial);
    m_ioCountSpin->setEnabled(true);
    m_modeCombo->setEnabled(true);
    m_offsetSpin->setEnabled(m_conversionMode == ConversionMode::Offset);
    m_autoOutputCheck->setEnabled(true);

    m_statusLabel->setText(QStringLiteral("状态：连接已取消"));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #64748b; font-weight: bold;"));
    appendLog(QStringLiteral("连接已取消"));
}

void MainWindow::setConnected(bool connected)
{
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_disconnectButton->setText(QStringLiteral("断开"));
    m_connectButton->setText(QStringLiteral("连接"));
    m_refreshButton->setEnabled(connected);
    m_connTypeCombo->setEnabled(!connected);
    m_ipEdit->setEnabled(!connected && currentConnectType() == ConnectType::Ethernet);
    m_comCombo->setEnabled(!connected && currentConnectType() == ConnectType::Serial);
    m_refreshPortButton->setEnabled(!connected && currentConnectType() == ConnectType::Serial);
    m_baudCombo->setEnabled(!connected && currentConnectType() == ConnectType::Serial);
    m_dataBitsCombo->setEnabled(!connected && currentConnectType() == ConnectType::Serial);
    m_parityCombo->setEnabled(!connected && currentConnectType() == ConnectType::Serial);
    m_stopBitsCombo->setEnabled(!connected && currentConnectType() == ConnectType::Serial);
    m_ioCountSpin->setEnabled(!connected);
    m_modeCombo->setEnabled(true);
    m_offsetSpin->setEnabled(m_conversionMode == ConversionMode::Offset);
    m_autoOutputCheck->setEnabled(true);
    m_connecting = false;

    if (connected) {
        m_statusLabel->setText(QStringLiteral("状态：已连接 %1").arg(m_pendingTarget));
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
}

void MainWindow::showConnectFailed(const QString &target, int errorCode)
{
    setConnected(false);
    m_statusLabel->setText(QStringLiteral("状态：连接失败（%1）").arg(target));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));

    appendLog(QStringLiteral("连接失败：无法连接到 %1，%2").arg(target, errorText(errorCode)));

    QString hint;
    if (m_pendingConnectType == ConnectType::Ethernet) {
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

ConnectType MainWindow::currentConnectType() const
{
    return static_cast<ConnectType>(m_connTypeCombo->currentData().toInt());
}

QString MainWindow::currentConnectTarget() const
{
    if (currentConnectType() == ConnectType::Ethernet) {
        return m_ipEdit->text().trimmed();
    }
    return m_comCombo->currentData().toString();
}

void MainWindow::updateConnectionUi()
{
    const ConnectType type = currentConnectType();
    m_targetStack->setCurrentIndex(type == ConnectType::Ethernet ? 0 : 1);
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

ZMotionConnectRequest MainWindow::buildConnectRequest()
{
    ZMotionConnectRequest request;
    request.type = currentConnectType();
    request.address = currentConnectTarget();
    request.baudRate = m_baudCombo->currentData().toInt();
    request.dataBits = m_dataBitsCombo->currentData().toInt();
    request.parity = m_parityCombo->currentData().toInt();
    request.stopBits = m_stopBitsCombo->currentData().toInt();
    request.cancelled = &m_connectCancelled;
    request.token = m_connectToken;
    return request;
}

void MainWindow::onConnectionTypeChanged(int index)
{
    Q_UNUSED(index)
    updateConnectionUi();
    if (!m_connecting && !m_handle) {
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
    if (!m_handle) {
        return false;
    }

    const int lastChannel = m_ioCount - 1;
    const int32_t outValue = static_cast<int32_t>(outputMask);
    const int rc = ZAux_Direct_SetOutMulti(m_handle, 0, lastChannel, outValue);
    if (rc != ERR_OK) {
        appendLog(QStringLiteral("写入输出失败：%1").arg(errorText(rc)));
        return false;
    }
    return true;
}

void MainWindow::onConnectClicked()
{
    const ConnectType type = currentConnectType();
    const QString target = currentConnectTarget();
    if (target.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("状态：连接失败"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
        if (type == ConnectType::Ethernet) {
            appendLog(QStringLiteral("连接失败：未输入 IP 地址"));
            QMessageBox::critical(this,
                                  QStringLiteral("连接失败"),
                                  QStringLiteral("连接失败，请输入 IO 板 IP 地址。"));
        } else {
            appendLog(QStringLiteral("连接失败：未选择串口"));
            QMessageBox::critical(this,
                                  QStringLiteral("连接失败"),
                                  QStringLiteral("连接失败，请选择串口。可点击「刷新」重新扫描串口。"));
        }
        return;
    }

    if (m_handle) {
        ZAux_Close(m_handle);
        m_handle = nullptr;
    }

    m_ignoreConnectResult = false;
    m_connectCancelled.storeRelease(0);
    m_pendingTarget = target;
    m_pendingConnectType = type;
    ++m_connectToken;

    const ZMotionConnectRequest request = buildConnectRequest();
    const QString typeLabel = connectTypeName(type);

    setConnecting(true, QStringLiteral("%1 %2").arg(typeLabel, target));
    appendLog(QStringLiteral("正在通过%1连接 %2 ...").arg(typeLabel, target));

    m_connectWatcher->setFuture(QtConcurrent::run(zmotionConnect, request));
}

void MainWindow::onConnectFinished()
{
    const ZMotionConnectResult result = m_connectWatcher->result();

    if (result.token != m_connectToken) {
        if (result.handle) {
            ZAux_Close(result.handle);
        }
        return;
    }

    if (m_ignoreConnectResult || result.errorCode == kZMotionConnectCancelled) {
        if (result.handle) {
            ZAux_Close(result.handle);
        }
        return;
    }

    setConnecting(false);

    if (result.errorCode != ERR_OK) {
        showConnectFailed(m_pendingTarget, result.errorCode);
        return;
    }

    m_handle = result.handle;
    m_ioCount = m_ioCountSpin->value();
    appendLog(QStringLiteral("已通过%1连接到 %2，IO 通道数 %3，转换模式：%4")
                  .arg(connectTypeName(m_pendingConnectType))
                  .arg(m_pendingTarget)
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
        rc = ZAux_Direct_GetOutMulti(m_handle, 0, lastChannel, &outputState);
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
