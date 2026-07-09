#include "sensorcircuitwidget.h"

#include <QPainter>
#include <QPainterPath>

SensorCircuitWidget::SensorCircuitWidget(QWidget *parent)
    : QWidget(parent)
    , m_type(SensorType::PNP)
    , m_triggered(false)
    , m_supplyVoltage(24)
{
    setMinimumHeight(220);
}

void SensorCircuitWidget::setSensorType(SensorType type)
{
    if (m_type == type) {
        return;
    }
    m_type = type;
    update();
}

void SensorCircuitWidget::setTriggered(bool triggered)
{
    if (m_triggered == triggered) {
        return;
    }
    m_triggered = triggered;
    update();
}

void SensorCircuitWidget::setSupplyVoltage(int voltage)
{
    if (m_supplyVoltage == voltage) {
        return;
    }
    m_supplyVoltage = voltage;
    update();
}

void SensorCircuitWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect area = rect().adjusted(16, 12, -16, -12);
    painter.fillRect(area, QColor(248, 250, 252));
    painter.setPen(QPen(QColor(203, 213, 225), 1));
    painter.drawRoundedRect(area, 8, 8);

    const int leftX = area.left() + 40;
    const int rightX = area.right() - 40;
    const int topY = area.top() + 36;
    const int bottomY = area.bottom() - 36;
    const int midY = (topY + bottomY) / 2;

    const LogicLevel level = outputLevel(m_type, m_triggered);
    const QColor wireColor = (level == LogicLevel::High) ? QColor(34, 197, 94) : QColor(148, 163, 184);
    const QColor activeColor = m_triggered ? QColor(239, 68, 68) : QColor(100, 116, 139);

    QFont labelFont = painter.font();
    labelFont.setPointSize(9);
    painter.setFont(labelFont);

    painter.setPen(QColor(30, 41, 59));
    painter.drawText(area.adjusted(12, 8, -12, 0), Qt::AlignTop | Qt::AlignLeft,
                     m_type == SensorType::PNP
                         ? QStringLiteral("PNP：触发时输出端接 +V，未触发时断开")
                         : QStringLiteral("NPN：触发时输出端接 GND，未触发时断开"));

    QPen wirePen(wireColor, 3, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(wirePen);

    painter.drawLine(leftX, topY, leftX + 80, topY);
    painter.drawLine(leftX, bottomY, leftX + 80, bottomY);

    painter.setPen(QColor(30, 41, 59));
    painter.drawText(leftX - 8, topY - 8, QStringLiteral("+%1V").arg(m_supplyVoltage));
    painter.drawText(leftX - 8, bottomY + 18, QStringLiteral("GND"));

    const QRect sensorRect(leftX + 90, midY - 34, 110, 68);
    painter.setBrush(QColor(241, 245, 249));
    painter.setPen(QPen(activeColor, 2));
    painter.drawRoundedRect(sensorRect, 10, 10);

    painter.setPen(QColor(30, 41, 59));
    painter.drawText(sensorRect, Qt::AlignCenter, QStringLiteral("传感器"));

    const int outputX = sensorRect.right() + 24;
    painter.setPen(wirePen);
    painter.drawLine(sensorRect.right(), midY, outputX, midY);

    if (m_triggered) {
        QPen linkPen(activeColor, 2, Qt::DashLine);
        painter.setPen(linkPen);
        if (m_type == SensorType::PNP) {
            painter.drawLine(sensorRect.left() + 20, topY, sensorRect.left() + 20, midY);
            painter.drawLine(sensorRect.left() + 20, topY, leftX + 80, topY);
        } else {
            painter.drawLine(sensorRect.left() + 20, bottomY, sensorRect.left() + 20, midY);
            painter.drawLine(sensorRect.left() + 20, bottomY, leftX + 80, bottomY);
        }
    }

    const QRect loadRect(outputX + 20, midY - 28, 90, 56);
    painter.setBrush(QColor(255, 255, 255));
    painter.setPen(QPen(wireColor, 2));
    painter.drawRoundedRect(loadRect, 8, 8);
    painter.setPen(QColor(30, 41, 59));
    painter.drawText(loadRect, Qt::AlignCenter, QStringLiteral("PLC\n输入"));

    painter.setPen(wirePen);
    painter.drawLine(outputX, midY, loadRect.left(), midY);

    if (m_type == SensorType::PNP) {
        painter.drawLine(loadRect.center().x(), loadRect.bottom(), loadRect.center().x(), bottomY);
        painter.drawLine(loadRect.center().x(), bottomY, leftX + 80, bottomY);
    } else {
        painter.drawLine(loadRect.center().x(), loadRect.top(), loadRect.center().x(), topY);
        QPen pullUpPen(QColor(251, 191, 36), 2, Qt::DashLine);
        painter.setPen(pullUpPen);
        painter.drawLine(loadRect.center().x(), topY, leftX + 80, topY);
        painter.setPen(QColor(180, 83, 9));
        painter.drawText(loadRect.center().x() + 8, topY - 6, QStringLiteral("上拉"));
    }

    const QRect ledRect(rightX - 70, midY - 16, 32, 32);
    painter.setBrush(level == LogicLevel::High ? QColor(34, 197, 94) : QColor(203, 213, 225));
    painter.setPen(QPen(QColor(30, 41, 59), 1));
    painter.drawEllipse(ledRect);

    painter.setPen(QColor(30, 41, 59));
    painter.drawText(ledRect.right() + 10, midY + 5,
                     level == LogicLevel::High
                         ? QStringLiteral("输出：高")
                         : QStringLiteral("输出：低"));
}
