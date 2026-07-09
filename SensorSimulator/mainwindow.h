#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "sensorsignal.h"

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;
class QTimer;
class SensorCircuitWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onSensorTypeChanged();
    void onTriggerClicked();
    void onReleaseClicked();
    void onToggleClicked();
    void onAutoModeToggled(bool enabled);
    void onPulseTimerTimeout();
    void onFrequencyChanged(int value);
    void onDutyCycleChanged(int value);
    void onSupplyVoltageChanged(int index);
    void updateOutputDisplay();

private:
    void setupUi();
    void setTriggered(bool triggered);

    SensorCircuitWidget *m_circuitWidget;
    QRadioButton *m_pnpRadio;
    QRadioButton *m_npnRadio;
    QComboBox *m_voltageCombo;
    QLabel *m_stateLabel;
    QLabel *m_levelLabel;
    QLabel *m_typeLabel;
    QPushButton *m_triggerButton;
    QPushButton *m_releaseButton;
    QPushButton *m_toggleButton;
    QCheckBox *m_autoModeCheck;
    QSpinBox *m_frequencySpin;
    QSlider *m_dutyCycleSlider;
    QLabel *m_dutyCycleLabel;
    QGroupBox *m_autoGroup;

    QTimer *m_pulseTimer;
    SensorType m_sensorType;
    bool m_triggered;
    int m_supplyVoltage;
    int m_pulseOnMs;
    bool m_pulsePhaseOn;
};

#endif // MAINWINDOW_H
