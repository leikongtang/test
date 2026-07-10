#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "signalconverter.h"
#include "connectionledwidget.h"
#include "zmotionconnector.h"
#include "zmcaux.h"

#include <QFutureWatcher>
#include <QMainWindow>
#include <QTimer>

#include <QAtomicInt>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QLabel;
class QWidget;
class IoPanelWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onRefreshClicked();
    void onPollTimeout();
    void onRefreshIntervalChanged(int value);
    void onIoCountChanged(int value);
    void onConversionModeChanged(int index);
    void onAutoOutputToggled(bool enabled);
    void onConnectFinished();
    void onConnectionModeChanged(int index);
    void onRefreshPortsClicked();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    void setConnected(bool connected);
    void setConnecting(bool connecting, const QString &target = QString());
    void cancelConnectingUi();
    void showConnectFailed(const QString &target, int errorCode);
    void updateConnectionUi();
    void updateConversionUi();
    void refreshSerialPortList();
    void appendLog(const QString &message);
    QString errorText(int errorCode) const;
    QString inputAddress() const;
    QString outputAddress() const;
    ConnectMode currentConnectMode() const;
    bool isConnected() const;
    void closeAllHandles();
    bool pollIoStates();
    bool writeOutputState(uint32_t outputMask);
    uint32_t applyConversion(uint32_t inputMask) const;
    void waitConnectFinished();
    void updateSerialLedState();
    void setConnectionInputsEnabled(bool enabled);
    ZMotionDualConnectRequest buildDualConnectRequest();

    ZMC_HANDLE m_inputHandle;
    ZMC_HANDLE m_outputHandle;
    QTimer *m_pollTimer;
    QFutureWatcher<ZMotionDualConnectResult> *m_connectWatcher;

    QComboBox *m_connModeCombo;
    QWidget *m_inputSection;
    QWidget *m_outputSection;
    QLineEdit *m_ipEdit;
    QComboBox *m_comCombo;
    QPushButton *m_refreshPortButton;
    QComboBox *m_baudCombo;
    QComboBox *m_parityCombo;
    QComboBox *m_dataBitsCombo;
    QComboBox *m_stopBitsCombo;
    QSpinBox *m_ioCountSpin;
    QSpinBox *m_refreshSpin;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QPushButton *m_refreshButton;
    QLabel *m_statusLabel;
    ConnectionLedWidget *m_serialLed;
    QPlainTextEdit *m_logEdit;
    IoPanelWidget *m_inputPanel;
    IoPanelWidget *m_convertedPanel;
    IoPanelWidget *m_outputPanel;

    QCheckBox *m_autoOutputCheck;
    QComboBox *m_modeCombo;
    QSpinBox *m_offsetSpin;
    QLabel *m_conversionDescLabel;

    ConnectMode m_pendingConnectMode;
    QString m_savedComPort;
    bool m_connecting;
    bool m_ignoreConnectResult;
    QAtomicInt m_connectCancelled;
    quint64 m_connectToken;

    int m_ioCount;
    ConversionMode m_conversionMode;
    int m_offset;
    int32_t m_lastInputState;
    uint32_t m_lastConvertedState;
    uint32_t m_lastOutputState;
};

#endif // MAINWINDOW_H
