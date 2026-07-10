#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "zmcaux.h"

#include <QMainWindow>
#include <QTimer>

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

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    void setConnected(bool connected);
    void appendLog(const QString &message);
    QString errorText(int errorCode) const;
    bool pollIoStates();

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
    IoPanelWidget *m_outputPanel;

    int m_ioCount;
    int32_t m_lastInputState;
    uint32_t m_lastOutputState;
};

#endif // MAINWINDOW_H
