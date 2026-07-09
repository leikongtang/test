#include "mainwindow.h"
#include "sensorcircuitwidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_circuitWidget(nullptr)
    , m_pnpRadio(nullptr)
    , m_npnRadio(nullptr)
    , m_voltageCombo(nullptr)
    , m_stateLabel(nullptr)
    , m_levelLabel(nullptr)
    , m_typeLabel(nullptr)
    , m_triggerButton(nullptr)
    , m_releaseButton(nullptr)
    , m_toggleButton(nullptr)
    , m_autoModeCheck(nullptr)
    , m_frequencySpin(nullptr)
    , m_dutyCycleSlider(nullptr)
    , m_dutyCycleLabel(nullptr)
    , m_autoGroup(nullptr)
    , m_pulseTimer(new QTimer(this))
    , m_sensorType(SensorType::PNP)
    , m_triggered(false)
    , m_supplyVoltage(24)
    , m_pulseOnMs(500)
    , m_pulsePhaseOn(true)
{
    setupUi();
    connect(m_pulseTimer, &QTimer::timeout, this, &MainWindow::onPulseTimerTimeout);
    updateOutputDisplay();
}

void MainWindow::setupUi()
{
    const QString version = QApplication::applicationVersion();
    setWindowTitle(QStringLiteral("传感器信号模拟器 v%1").arg(version));
    resize(760, 620);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(12);

    QGroupBox *typeGroup = new QGroupBox(QStringLiteral("传感器类型"), central);
    QHBoxLayout *typeLayout = new QHBoxLayout(typeGroup);

    m_pnpRadio = new QRadioButton(QStringLiteral("PNP（源型 / 高电平有效）"), typeGroup);
    m_npnRadio = new QRadioButton(QStringLiteral("NPN（漏型 / 低电平有效）"), typeGroup);
    m_pnpRadio->setChecked(true);

    typeLayout->addWidget(m_pnpRadio);
    typeLayout->addWidget(m_npnRadio);
    typeLayout->addStretch();

    QLabel *voltageLabel = new QLabel(QStringLiteral("供电电压："), typeGroup);
    m_voltageCombo = new QComboBox(typeGroup);
    m_voltageCombo->addItem(QStringLiteral("24 V"), 24);
    m_voltageCombo->addItem(QStringLiteral("12 V"), 12);
    m_voltageCombo->addItem(QStringLiteral("5 V"), 5);
    typeLayout->addWidget(voltageLabel);
    typeLayout->addWidget(m_voltageCombo);

    m_circuitWidget = new SensorCircuitWidget(central);

    QGroupBox *statusGroup = new QGroupBox(QStringLiteral("输出状态"), central);
    QGridLayout *statusLayout = new QGridLayout(statusGroup);

    statusLayout->addWidget(new QLabel(QStringLiteral("当前类型："), statusGroup), 0, 0);
    m_typeLabel = new QLabel(statusGroup);
    statusLayout->addWidget(m_typeLabel, 0, 1);

    statusLayout->addWidget(new QLabel(QStringLiteral("触发状态："), statusGroup), 1, 0);
    m_stateLabel = new QLabel(statusGroup);
    statusLayout->addWidget(m_stateLabel, 1, 1);

    statusLayout->addWidget(new QLabel(QStringLiteral("输出电平："), statusGroup), 2, 0);
    m_levelLabel = new QLabel(statusGroup);
    statusLayout->addWidget(m_levelLabel, 2, 1);

    QGroupBox *controlGroup = new QGroupBox(QStringLiteral("信号控制"), central);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);

    m_triggerButton = new QPushButton(QStringLiteral("触发（检测到物体）"), controlGroup);
    m_releaseButton = new QPushButton(QStringLiteral("释放（未检测到）"), controlGroup);
    m_toggleButton = new QPushButton(QStringLiteral("切换状态"), controlGroup);

    m_triggerButton->setMinimumHeight(36);
    m_releaseButton->setMinimumHeight(36);
    m_toggleButton->setMinimumHeight(36);

    controlLayout->addWidget(m_triggerButton);
    controlLayout->addWidget(m_releaseButton);
    controlLayout->addWidget(m_toggleButton);

    m_autoGroup = new QGroupBox(QStringLiteral("自动脉冲模式"), central);
    QGridLayout *autoLayout = new QGridLayout(m_autoGroup);

    m_autoModeCheck = new QCheckBox(QStringLiteral("启用自动脉冲"), m_autoGroup);
    autoLayout->addWidget(m_autoModeCheck, 0, 0, 1, 2);

    autoLayout->addWidget(new QLabel(QStringLiteral("频率 (Hz)："), m_autoGroup), 1, 0);
    m_frequencySpin = new QSpinBox(m_autoGroup);
    m_frequencySpin->setRange(1, 50);
    m_frequencySpin->setValue(1);
    m_frequencySpin->setEnabled(false);
    autoLayout->addWidget(m_frequencySpin, 1, 1);

    autoLayout->addWidget(new QLabel(QStringLiteral("占空比 (%)："), m_autoGroup), 2, 0);
    m_dutyCycleSlider = new QSlider(Qt::Horizontal, m_autoGroup);
    m_dutyCycleSlider->setRange(10, 90);
    m_dutyCycleSlider->setValue(50);
    m_dutyCycleSlider->setEnabled(false);
    m_dutyCycleLabel = new QLabel(QStringLiteral("50%"), m_autoGroup);
    autoLayout->addWidget(m_dutyCycleSlider, 2, 1);
    autoLayout->addWidget(m_dutyCycleLabel, 2, 2);

    mainLayout->addWidget(typeGroup);
    mainLayout->addWidget(m_circuitWidget, 1);
    mainLayout->addWidget(statusGroup);
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(m_autoGroup);

    setCentralWidget(central);

    connect(m_pnpRadio, &QRadioButton::toggled, this, &MainWindow::onSensorTypeChanged);
    connect(m_voltageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSupplyVoltageChanged);
    connect(m_triggerButton, &QPushButton::clicked, this, &MainWindow::onTriggerClicked);
    connect(m_releaseButton, &QPushButton::clicked, this, &MainWindow::onReleaseClicked);
    connect(m_toggleButton, &QPushButton::clicked, this, &MainWindow::onToggleClicked);
    connect(m_autoModeCheck, &QCheckBox::toggled, this, &MainWindow::onAutoModeToggled);
    connect(m_frequencySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFrequencyChanged);
    connect(m_dutyCycleSlider, &QSlider::valueChanged, this, &MainWindow::onDutyCycleChanged);
}

void MainWindow::onSensorTypeChanged()
{
    m_sensorType = m_pnpRadio->isChecked() ? SensorType::PNP : SensorType::NPN;
    m_circuitWidget->setSensorType(m_sensorType);
    updateOutputDisplay();
}

void MainWindow::onTriggerClicked()
{
    setTriggered(true);
}

void MainWindow::onReleaseClicked()
{
    setTriggered(false);
}

void MainWindow::onToggleClicked()
{
    setTriggered(!m_triggered);
}

void MainWindow::onAutoModeToggled(bool enabled)
{
    m_frequencySpin->setEnabled(enabled);
    m_dutyCycleSlider->setEnabled(enabled);
    m_triggerButton->setEnabled(!enabled);
    m_releaseButton->setEnabled(!enabled);
    m_toggleButton->setEnabled(!enabled);

    if (enabled) {
        onFrequencyChanged(m_frequencySpin->value());
        m_pulsePhaseOn = true;
        setTriggered(true);
        m_pulseTimer->start(m_pulseOnMs);
    } else {
        m_pulseTimer->stop();
        setTriggered(false);
    }
}

void MainWindow::onPulseTimerTimeout()
{
    m_pulsePhaseOn = !m_pulsePhaseOn;
    setTriggered(m_pulsePhaseOn);

    const int frequency = m_frequencySpin->value();
    const int periodMs = 1000 / frequency;
    const int onMs = periodMs * m_dutyCycleSlider->value() / 100;
    const int offMs = periodMs - onMs;
    m_pulseTimer->start(m_pulsePhaseOn ? onMs : offMs);
}

void MainWindow::onFrequencyChanged(int value)
{
    Q_UNUSED(value)
    if (!m_autoModeCheck->isChecked()) {
        return;
    }

    const int frequency = m_frequencySpin->value();
    const int periodMs = 1000 / frequency;
    m_pulseOnMs = periodMs * m_dutyCycleSlider->value() / 100;

    if (m_pulseTimer->isActive()) {
        m_pulseTimer->stop();
        m_pulsePhaseOn = true;
        setTriggered(true);
        m_pulseTimer->start(m_pulseOnMs);
    }
}

void MainWindow::onDutyCycleChanged(int value)
{
    m_dutyCycleLabel->setText(QStringLiteral("%1%").arg(value));
    onFrequencyChanged(m_frequencySpin->value());
}

void MainWindow::onSupplyVoltageChanged(int index)
{
    m_supplyVoltage = m_voltageCombo->itemData(index).toInt();
    m_circuitWidget->setSupplyVoltage(m_supplyVoltage);
    updateOutputDisplay();
}

void MainWindow::setTriggered(bool triggered)
{
    m_triggered = triggered;
    m_circuitWidget->setTriggered(triggered);
    updateOutputDisplay();
}

void MainWindow::updateOutputDisplay()
{
    const LogicLevel level = outputLevel(m_sensorType, m_triggered);

    m_typeLabel->setText(sensorTypeName(m_sensorType));
    m_stateLabel->setText(m_triggered
                              ? QStringLiteral("已触发（检测到物体）")
                              : QStringLiteral("未触发（空闲）"));
    m_levelLabel->setText(levelText(level, m_supplyVoltage));

    const QString levelStyle = (level == LogicLevel::High)
        ? QStringLiteral("color: #15803d; font-weight: bold;")
        : QStringLiteral("color: #64748b; font-weight: bold;");
    m_levelLabel->setStyleSheet(levelStyle);

    const QString stateStyle = m_triggered
        ? QStringLiteral("color: #dc2626; font-weight: bold;")
        : QStringLiteral("color: #475569;");
    m_stateLabel->setStyleSheet(stateStyle);
}
