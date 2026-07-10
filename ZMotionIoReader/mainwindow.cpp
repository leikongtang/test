#include "mainwindow.h"
#include "iopanelwidget.h"

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

namespace {
const char kSettingsOrg[] = "LktCodes";
const char kSettingsApp[] = "ZMotionIoReader";
const char kKeyIp[] = "controller/ip";
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
    , m_ipEdit(nullptr)
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
    setWindowTitle(QStringLiteral("正运动 IO 信号收发转换 v%1").arg(version));
    resize(780, 860);

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
}

void MainWindow::loadSettings()
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    m_ipEdit->setText(settings.value(kKeyIp, QStringLiteral("192.168.0.11")).toString());
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
    settings.setValue(kKeyIp, m_ipEdit->text().trimmed());
    settings.setValue(kSettingsIoCount, m_ioCountSpin->value());
    settings.setValue(kKeyRefreshMs, m_refreshSpin->value());
    settings.setValue(kKeyAutoOutput, m_autoOutputCheck->isChecked());
    settings.setValue(kKeyConversionMode, m_modeCombo->currentData().toInt());
    settings.setValue(kKeyOffset, m_offsetSpin->value());
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
        m_statusLabel->setStyleSheet(QStringLiteral("color: #64748b; font-weight: bold;"));
        m_inputPanel->setAllStates(0);
        m_convertedPanel->setAllStates(0);
        m_outputPanel->setAllStates(0);
        m_lastInputState = 0;
        m_lastConvertedState = 0;
        m_lastOutputState = 0;
    }
}

void MainWindow::showConnectFailed(const QString &ip, int errorCode)
{
    setConnected(false);
    m_statusLabel->setText(QStringLiteral("状态：连接失败（%1）").arg(ip));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));

    appendLog(QStringLiteral("连接失败：无法连接到 %1，%2").arg(ip, errorText(errorCode)));
    QMessageBox::critical(this,
                          QStringLiteral("连接失败"),
                          QStringLiteral("连接失败，无法连接到 IO 板 %1。\n\n请检查：\n"
                                         "1. IP 地址是否正确\n"
                                         "2. 网线是否已连接\n"
                                         "3. PC 与 IO 板是否在同一网段\n\n"
                                         "%2")
                              .arg(ip, errorText(errorCode)));
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
    const QString ip = m_ipEdit->text().trimmed();
    if (ip.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("状态：连接失败"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
        appendLog(QStringLiteral("连接失败：未输入 IO 板 IP 地址"));
        QMessageBox::critical(this,
                              QStringLiteral("连接失败"),
                              QStringLiteral("连接失败，请输入 IO 板 IP 地址。"));
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
        showConnectFailed(ip, rc);
        return;
    }

    m_ioCount = m_ioCountSpin->value();
    appendLog(QStringLiteral("已连接到 %1，IO 通道数 %2，转换模式：%3")
                  .arg(ip)
                  .arg(m_ioCount)
                  .arg(SignalConverter::modeName(m_conversionMode)));
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
