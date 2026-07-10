#ifndef CONNECTIONLEDWIDGET_H
#define CONNECTIONLEDWIDGET_H

#include <QWidget>

class QLabel;
class QTimer;

class ConnectionLedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionLedWidget(QWidget *parent = nullptr);

    void setConnected(bool connected);
    void setVisibleForSerial(bool visible);

private slots:
    void onBlinkTimeout();

private:
    void applyDisconnectedStyle();
    void applyConnectedBrightStyle();
    void applyConnectedDimStyle();

    QLabel *m_ledLabel;
    QLabel *m_textLabel;
    QTimer *m_blinkTimer;
    bool m_connected;
    bool m_blinkOn;
};

#endif // CONNECTIONLEDWIDGET_H
