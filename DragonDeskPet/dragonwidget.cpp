#include "dragonwidget.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>

DragonWidget::DragonWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(220, 220);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    connect(&m_animTimer, &QTimer::timeout, this, &DragonWidget::onAnimTick);
    m_animTimer.start(33);

    connect(&m_blinkTimer, &QTimer::timeout, this, &DragonWidget::onBlink);
    scheduleNextBlink();
}

void DragonWidget::onAnimTick()
{
    m_time += 0.033;

    m_breathScale = 1.0 + 0.03 * qSin(m_time * 2.0);
    m_tailAngle = 12.0 * qSin(m_time * 3.5);
    m_wingAngle = 18.0 * qSin(m_time * 5.0);
    m_bounceY = 4.0 * qSin(m_time * 1.8);

    if (m_breathingFire) {
        m_fireIntensity = qMin(1.0, m_fireIntensity + 0.08);
        if (m_time - m_fireStartTime > 1.5) {
            m_breathingFire = false;
        }
    } else {
        m_fireIntensity = qMax(0.0, m_fireIntensity - 0.06);
    }

    update();
}

void DragonWidget::onBlink()
{
    m_eyesClosed = true;
    update();
    QTimer::singleShot(120, this, [this]() {
        m_eyesClosed = false;
        update();
        scheduleNextBlink();
    });
}

void DragonWidget::scheduleNextBlink()
{
    m_blinkTimer.start(2000 + qrand() % 3000);
}

void DragonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.translate(width() / 2.0, height() / 2.0 + m_bounceY);
    painter.scale(m_breathScale, m_breathScale);

    drawDragon(painter);

    if (m_fireIntensity > 0.01) {
        drawFire(painter, m_fireIntensity);
    }
}

void DragonWidget::drawDragon(QPainter &painter)
{
    const qreal s = 1.0;

    painter.save();

    // 尾巴
    painter.save();
    painter.translate(-38 * s, 18 * s);
    painter.rotate(m_tailAngle);
    QPainterPath tail;
    tail.moveTo(0, 0);
    tail.cubicTo(-30 * s, 8 * s, -55 * s, -5 * s, -72 * s, 12 * s);
    tail.cubicTo(-58 * s, 16 * s, -40 * s, 10 * s, -20 * s, 6 * s);
    tail.closeSubpath();
    QLinearGradient tailGrad(-72 * s, 0, 0, 0);
    tailGrad.setColorAt(0.0, QColor(46, 139, 122));
    tailGrad.setColorAt(1.0, QColor(56, 168, 148));
    painter.setBrush(tailGrad);
    painter.setPen(QPen(QColor(30, 100, 88), 1.5));
    painter.drawPath(tail);

    // 尾刺
    painter.setBrush(QColor(255, 183, 77));
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < 3; ++i) {
        const qreal tx = (-25 - i * 18) * s;
        const qreal ty = (6 + (i % 2) * 4) * s;
        QPainterPath spike;
        spike.moveTo(tx, ty);
        spike.lineTo(tx - 6 * s, ty + 10 * s);
        spike.lineTo(tx + 6 * s, ty + 10 * s);
        spike.closeSubpath();
        painter.drawPath(spike);
    }
    painter.restore();

    // 后翅
    painter.save();
    painter.translate(-8 * s, -8 * s);
    painter.rotate(-m_wingAngle * 0.6);
    QPainterPath backWing;
    backWing.moveTo(0, 0);
    backWing.cubicTo(-18 * s, -28 * s, -8 * s, -52 * s, 12 * s, -38 * s);
    backWing.cubicTo(8 * s, -18 * s, 6 * s, -6 * s, 0, 0);
    painter.setBrush(QColor(72, 160, 140, 180));
    painter.setPen(QPen(QColor(40, 110, 98), 1.2));
    painter.drawPath(backWing);
    painter.restore();

    // 身体
    QPainterPath body;
    body.addEllipse(QPointF(0, 8 * s), 34 * s, 30 * s);
    QRadialGradient bodyGrad(0, 4 * s, 40 * s);
    bodyGrad.setColorAt(0.0, QColor(80, 190, 168));
    bodyGrad.setColorAt(0.6, QColor(46, 139, 122));
    bodyGrad.setColorAt(1.0, QColor(36, 110, 98));
    painter.setBrush(bodyGrad);
    painter.setPen(QPen(QColor(30, 95, 85), 1.5));
    painter.drawPath(body);

    // 肚皮
    QPainterPath belly;
    belly.addEllipse(QPointF(0, 14 * s), 20 * s, 18 * s);
    painter.setBrush(QColor(255, 224, 130));
    painter.setPen(Qt::NoPen);
    painter.drawPath(belly);

    // 前翅
    painter.save();
    painter.translate(6 * s, -6 * s);
    painter.rotate(m_wingAngle);
    QPainterPath frontWing;
    frontWing.moveTo(0, 0);
    frontWing.cubicTo(22 * s, -30 * s, 48 * s, -22 * s, 38 * s, 2 * s);
    frontWing.cubicTo(24 * s, -4 * s, 10 * s, -2 * s, 0, 0);
    QLinearGradient wingGrad(0, -30 * s, 40 * s, 0);
    wingGrad.setColorAt(0.0, QColor(100, 210, 185));
    wingGrad.setColorAt(1.0, QColor(56, 168, 148));
    painter.setBrush(wingGrad);
    painter.setPen(QPen(QColor(40, 110, 98), 1.2));
    painter.drawPath(frontWing);
    painter.restore();

    // 后腿
    painter.setBrush(QColor(46, 139, 122));
    painter.setPen(QPen(QColor(30, 95, 85), 1.2));
    painter.drawEllipse(QRectF(-22 * s, 28 * s, 14 * s, 10 * s));
    painter.drawEllipse(QRectF(8 * s, 28 * s, 14 * s, 10 * s));

    // 脖子
    QPainterPath neck;
    neck.addEllipse(QPointF(0, -18 * s), 22 * s, 20 * s);
    painter.setBrush(bodyGrad);
    painter.setPen(QPen(QColor(30, 95, 85), 1.5));
    painter.drawPath(neck);

    // 头
    QPainterPath head;
    head.addEllipse(QPointF(0, -38 * s), 30 * s, 28 * s);
    painter.setBrush(bodyGrad);
    painter.drawPath(head);

    // 吻部
    QPainterPath snout;
    snout.addEllipse(QPointF(0, -30 * s), 18 * s, 14 * s);
    painter.setBrush(QColor(90, 200, 175));
    painter.setPen(Qt::NoPen);
    painter.drawPath(snout);

    // 鼻孔
    painter.setBrush(QColor(30, 80, 70));
    painter.drawEllipse(QRectF(-5 * s, -32 * s, 4 * s, 3 * s));
    painter.drawEllipse(QRectF(1 * s, -32 * s, 4 * s, 3 * s));

    // 角
    painter.setBrush(QColor(255, 183, 77));
    painter.setPen(QPen(QColor(220, 150, 50), 1.0));
    QPainterPath hornL;
    hornL.moveTo(-14 * s, -52 * s);
    hornL.lineTo(-20 * s, -72 * s);
    hornL.lineTo(-8 * s, -54 * s);
    hornL.closeSubpath();
    painter.drawPath(hornL);
    QPainterPath hornR;
    hornR.moveTo(14 * s, -52 * s);
    hornR.lineTo(20 * s, -72 * s);
    hornR.lineTo(8 * s, -54 * s);
    hornR.closeSubpath();
    painter.drawPath(hornR);

    // 耳鳍
    painter.setBrush(QColor(255, 138, 101));
    painter.setPen(Qt::NoPen);
    QPainterPath finL;
    finL.moveTo(-24 * s, -48 * s);
    finL.lineTo(-32 * s, -58 * s);
    finL.lineTo(-18 * s, -46 * s);
    finL.closeSubpath();
    painter.drawPath(finL);
    QPainterPath finR;
    finR.moveTo(24 * s, -48 * s);
    finR.lineTo(32 * s, -58 * s);
    finR.lineTo(18 * s, -46 * s);
    finR.closeSubpath();
    painter.drawPath(finR);

    // 眼睛
    const qreal eyeY = -42 * s;
    const qreal eyeRX = 11 * s;
    const qreal eyeLX = -11 * s;
    if (m_eyesClosed) {
        painter.setPen(QPen(QColor(30, 60, 55), 2.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(eyeLX - 7 * s, eyeY), QPointF(eyeLX + 7 * s, eyeY));
        painter.drawLine(QPointF(eyeRX - 7 * s, eyeY), QPointF(eyeRX + 7 * s, eyeY));
    } else {
        painter.setBrush(Qt::white);
        painter.setPen(QPen(QColor(30, 60, 55), 1.2));
        painter.drawEllipse(QPointF(eyeLX, eyeY), 9 * s, 10 * s);
        painter.drawEllipse(QPointF(eyeRX, eyeY), 9 * s, 10 * s);

        const qreal pupilOffsetX = 2.0 * qSin(m_time * 0.8);
        const qreal pupilOffsetY = 1.0 * qCos(m_time * 0.6);
        painter.setBrush(QColor(25, 45, 40));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(eyeLX + pupilOffsetX, eyeY + pupilOffsetY), 4.5 * s, 5 * s);
        painter.drawEllipse(QPointF(eyeRX + pupilOffsetX, eyeY + pupilOffsetY), 4.5 * s, 5 * s);

        painter.setBrush(Qt::white);
        painter.drawEllipse(QPointF(eyeLX + pupilOffsetX - 2 * s, eyeY + pupilOffsetY - 2 * s), 2 * s, 2 * s);
        painter.drawEllipse(QPointF(eyeRX + pupilOffsetX - 2 * s, eyeY + pupilOffsetY - 2 * s), 2 * s, 2 * s);
    }

    // 前爪
    painter.setBrush(QColor(56, 168, 148));
    painter.setPen(QPen(QColor(30, 95, 85), 1.2));
    painter.drawEllipse(QRectF(-28 * s, 10 * s, 12 * s, 10 * s));
    painter.drawEllipse(QRectF(16 * s, 10 * s, 12 * s, 10 * s));

    // 背脊
    painter.setBrush(QColor(255, 183, 77));
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < 4; ++i) {
        const qreal dx = (-12 + i * 8) * s;
        const qreal dy = (-2 + (i % 2) * 2) * s;
        QPainterPath ridge;
        ridge.moveTo(dx, dy);
        ridge.lineTo(dx - 3 * s, dy - 8 * s);
        ridge.lineTo(dx + 3 * s, dy - 8 * s);
        ridge.closeSubpath();
        painter.drawPath(ridge);
    }

    painter.restore();
}

void DragonWidget::drawFire(QPainter &painter, qreal intensity)
{
    painter.save();
    painter.translate(0, -24);

    const int flameCount = 5;
    for (int i = 0; i < flameCount; ++i) {
        const qreal spread = (i - flameCount / 2.0) * 8.0;
        const qreal flicker = qSin(m_time * 12.0 + i * 1.3) * 4.0;
        const qreal flameH = (28.0 + flicker) * intensity;
        const qreal flameW = (10.0 + qAbs(spread) * 0.3) * intensity;

        QPainterPath flame;
        flame.moveTo(spread - flameW * 0.5, -30);
        flame.cubicTo(spread - flameW, -30 - flameH * 0.5,
                      spread - flameW * 0.3, -30 - flameH,
                      spread, -30 - flameH * 1.1);
        flame.cubicTo(spread + flameW * 0.3, -30 - flameH,
                      spread + flameW, -30 - flameH * 0.5,
                      spread + flameW * 0.5, -30);
        flame.closeSubpath();

        QLinearGradient grad(spread, -30, spread, -30 - flameH);
        grad.setColorAt(0.0, QColor(255, 235, 59, int(220 * intensity)));
        grad.setColorAt(0.4, QColor(255, 152, 0, int(200 * intensity)));
        grad.setColorAt(1.0, QColor(244, 67, 54, int(80 * intensity)));

        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawPath(flame);
    }

    painter.restore();
}

void DragonWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void DragonWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragOffset);
        event->accept();
    }
}

void DragonWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

void DragonWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_breathingFire = true;
        m_fireIntensity = 0.0;
        m_fireStartTime = m_time;
        event->accept();
    }
}

void DragonWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *quitAction = menu.addAction(QStringLiteral("退出"));
    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == quitAction) {
        qApp->quit();
    }
}
