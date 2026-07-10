#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "signalconverter.h"
#include "zmcaux.h"

#include <QMainWindow>
#include <QTimer>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QLabel;
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

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    void setConnected(bool connected);
    void showConnectFailed(const QString &ip, int errorCode);
    void updateConversionUi();
    void appendLog(const QString &message);
    QString errorText(int errorCode) const;
    bool pollIoStates();
    bool writeOutputState(uint32_t outputMask);
    uint32_t applyConversion(uint32_t inputMask) const;

    ZMC_HANDLE m_handle;
    QTimer *m_pollTimer;

    QLineEdit *m_ipEdit;
    QSpinBox *m_ioCountSpin;
    QSpinBox *m_refreshSpin;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QPushButton *m_refreshButton;
    QLabel *m_statusLabel;
    QPlainTextEdit *m_logEdit;
    IoPanelWidget *m_inputPanel;
    IoPanelWidget *m_convertedPanel;
    IoPanelWidget *m_outputPanel;

    QCheckBox *m_autoOutputCheck;
    QComboBox *m_modeCombo;
    QSpinBox *m_offsetSpin;
    QLabel *m_conversionDescLabel;

    int m_ioCount;
    ConversionMode m_conversionMode;
    int m_offset;
    int32_t m_lastInputState;
    uint32_t m_lastConvertedState;
    uint32_t m_lastOutputState;
};

#endif // MAINWINDOW_H
