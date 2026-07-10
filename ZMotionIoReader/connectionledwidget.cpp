#include "connectionledwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

namespace {
const QString kLedBaseStyle = QStringLiteral(
    "QLabel { border: 1px solid #94a3b8; border-radius: 9px; min-width: 18px; max-width: 18px;"
    " min-height: 18px; max-height: 18px; }");
}

ConnectionLedWidget::ConnectionLedWidget(QWidget *parent)
    : QWidget(parent)
    , m_ledLabel(new QLabel(this))
    , m_textLabel(new QLabel(QStringLiteral("未连接"), this))
    , m_blinkTimer(new QTimer(this))
    , m_connected(false)
    , m_blinkOn(true)
{
    m_ledLabel->setAlignment(Qt::AlignCenter);
    m_textLabel->setStyleSheet(QStringLiteral("color: #64748b;"));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_ledLabel);
    layout->addWidget(m_textLabel);
    layout->addStretch();

    connect(m_blinkTimer, &QTimer::timeout, this, &ConnectionLedWidget::onBlinkTimeout);
    m_blinkTimer->setInterval(500);

    setConnected(false);
}

void ConnectionLedWidget::setVisibleForSerial(bool visible)
{
    QWidget::setVisible(visible);
}

void ConnectionLedWidget::setConnected(bool connected)
{
    m_connected = connected;
    m_blinkTimer->stop();

    if (connected) {
        m_textLabel->setText(QStringLiteral("已连接"));
        m_textLabel->setStyleSheet(QStringLiteral("color: #15803d; font-weight: bold;"));
        m_blinkOn = true;
        applyConnectedBrightStyle();
        m_blinkTimer->start();
    } else {
        m_textLabel->setText(QStringLiteral("未连接"));
        m_textLabel->setStyleSheet(QStringLiteral("color: #dc2626; font-weight: bold;"));
        applyDisconnectedStyle();
    }
}

void ConnectionLedWidget::onBlinkTimeout()
{
    if (!m_connected) {
        return;
    }

    m_blinkOn = !m_blinkOn;
    if (m_blinkOn) {
        applyConnectedBrightStyle();
    } else {
        applyConnectedDimStyle();
    }
}

void ConnectionLedWidget::applyDisconnectedStyle()
{
    m_ledLabel->setStyleSheet(kLedBaseStyle + QStringLiteral(" { background-color: #dc2626; }"));
}

void ConnectionLedWidget::applyConnectedBrightStyle()
{
    m_ledLabel->setStyleSheet(kLedBaseStyle + QStringLiteral(" { background-color: #22c55e; }"));
}

void ConnectionLedWidget::applyConnectedDimStyle()
{
    m_ledLabel->setStyleSheet(kLedBaseStyle + QStringLiteral(" { background-color: #15803d; }"));
}
