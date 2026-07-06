#include "dragonwidget.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QCursor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLineF>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QScreen>
#include <QtMath>

namespace {

const char *kRandomLines[] = {
    "今天也要加油哦~",
    "龙龙在此，诸邪退散！",
    "摸我头会喷火……开玩笑的~",
    "肚子饿了，要喂食！",
    "我在看什么呢？",
    "陪你一起摸鱼~",
    "呼~呼~",
    "尾巴不许拽！"
};

} // namespace

DragonWidget::DragonWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(240, 260);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    connect(&m_animTimer, &QTimer::timeout, this, &DragonWidget::onAnimTick);
    m_animTimer.start(33);

    connect(&m_blinkTimer, &QTimer::timeout, this, &DragonWidget::onBlink);
    scheduleNextBlink();

    connect(&m_idleTimer, &QTimer::timeout, this, &DragonWidget::onIdleCheck);
    m_idleTimer.start(1000);

    showBubble(QStringLiteral("你好，我是龙龙！"), 3000);
}

QPointF DragonWidget::toDragonLocal(const QPoint &widgetPos) const
{
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0 + 10.0 + m_bounceY;
    const qreal x = (widgetPos.x() - cx) / m_breathScale;
    const qreal y = (widgetPos.y() - cy) / m_breathScale;
    return QPointF(x, y);
}

DragonWidget::HitRegion DragonWidget::hitTest(const QPointF &local) const
{
    const QPointF headCenter(0, -38);
    if (QLineF(local, headCenter).length() < 32.0) {
        return HitRegion::Head;
    }

    const QPointF bodyCenter(0, 8);
    if (QLineF(local, bodyCenter).length() < 36.0) {
        return HitRegion::Body;
    }

    if (local.x() < -20 && local.x() > -90 && local.y() > -5 && local.y() < 35) {
        return HitRegion::Tail;
    }

    if (local.x() > -50 && local.x() < 50 && local.y() > -60 && local.y() < 45) {
        return HitRegion::Body;
    }

    return HitRegion::None;
}

void DragonWidget::setMood(Mood mood)
{
    m_mood = mood;
    if (mood == Mood::Happy) {
        m_happyTimer = 2.5;
        m_blushAlpha = 1.0;
    } else if (mood == Mood::Normal) {
        m_blushAlpha = 0.0;
    }
}

void DragonWidget::wakeUp()
{
    if (m_mood == Mood::Sleeping) {
        setMood(Mood::Happy);
        showBubble(QStringLiteral("嗯？谁叫我~"), 2000);
        m_idleSeconds = 0.0;
    }
}

void DragonWidget::showBubble(const QString &text, int durationMs)
{
    m_bubbleText = text;
    m_bubbleAlpha = 1.0;
    m_bubbleTimer = durationMs / 1000.0;
}

void DragonWidget::spawnHearts(int count)
{
    for (int i = 0; i < count; ++i) {
        Heart h;
        h.pos = QPointF(-15.0 + (qrand() % 30), -50.0 + (qrand() % 20));
        h.life = 1.0;
        h.vy = -35.0 - (qrand() % 20);
        m_hearts.append(h);
    }
}

void DragonWidget::triggerInteraction(HitRegion region)
{
    wakeUp();
    m_idleSeconds = 0.0;

    switch (region) {
    case HitRegion::Head:
        setMood(Mood::Happy);
        spawnHearts(4);
        showBubble(QStringLiteral("摸摸头，好开心~"));
        m_headTilt = (m_localMouse.x() > 0 ? 8.0 : -8.0);
        break;
    case HitRegion::Body:
        setMood(Mood::Happy);
        spawnHearts(3);
        showBubble(QStringLiteral("肚肚好舒服~"));
        m_bounceY -= 8.0;
        break;
    case HitRegion::Tail:
        setMood(Mood::Happy);
        m_tailAngle = 25.0;
        showBubble(QStringLiteral("尾巴会摇得更欢哦！"));
        break;
    default:
        break;
    }
}

void DragonWidget::feedDragon()
{
    wakeUp();
    m_idleSeconds = 0.0;
    setMood(Mood::Eating);
    m_eatTimer = 2.0;
    m_foods.clear();
    Food food;
    food.pos = QPointF(0, -90);
    food.vy = 0.0;
    m_foods.append(food);
    showBubble(QStringLiteral("啊呜啊呜~"), 2000);
}

void DragonWidget::sayRandomLine()
{
    wakeUp();
    const int count = int(sizeof(kRandomLines) / sizeof(kRandomLines[0]));
    showBubble(QString::fromUtf8(kRandomLines[qrand() % count]));
}

void DragonWidget::updateHearts(qreal dt)
{
    for (int i = m_hearts.size() - 1; i >= 0; --i) {
        Heart &h = m_hearts[i];
        h.pos.setY(h.pos.y() + h.vy * dt);
        h.life -= dt * 0.8;
        if (h.life <= 0.0) {
            m_hearts.removeAt(i);
        }
    }
}

void DragonWidget::updateFood(qreal dt)
{
    for (Food &food : m_foods) {
        if (food.eaten) {
            continue;
        }
        food.vy += 180.0 * dt;
        food.pos.setY(food.pos.y() + food.vy * dt);
        if (food.pos.y() >= -28.0) {
            food.eaten = true;
            m_bounceY -= 5.0;
        }
    }
}

void DragonWidget::updateFollowMouse(qreal dt)
{
    if (!m_followMouse || m_dragging || m_mood == Mood::Sleeping) {
        return;
    }

    const QPoint globalCenter = frameGeometry().center();
    const QPoint cursor = QCursor::pos();
    const QLineF line(globalCenter, cursor);
    if (line.length() < 80.0) {
        return;
    }

    const qreal step = qMin(120.0 * dt, line.length() - 60.0);
    const qreal ratio = step / line.length();
    const QPoint delta(int((cursor.x() - globalCenter.x()) * ratio),
                       int((cursor.y() - globalCenter.y()) * ratio));
    move(pos() + delta);
}

void DragonWidget::updateMood(qreal dt)
{
    if (m_happyTimer > 0.0) {
        m_happyTimer -= dt;
        if (m_happyTimer <= 0.0 && m_mood == Mood::Happy) {
            setMood(Mood::Normal);
        }
    }

    if (m_eatTimer > 0.0) {
        m_eatTimer -= dt;
        if (m_eatTimer <= 0.0 && m_mood == Mood::Eating) {
            setMood(Mood::Happy);
            spawnHearts(2);
        }
    }

    if (m_blushAlpha > 0.0 && m_mood != Mood::Happy) {
        m_blushAlpha = qMax(0.0, m_blushAlpha - dt * 2.0);
    }

    if (m_bubbleTimer > 0.0) {
        m_bubbleTimer -= dt;
        if (m_bubbleTimer <= 0.0) {
            m_bubbleAlpha = qMax(0.0, m_bubbleAlpha - dt * 2.0);
        }
    } else if (m_bubbleAlpha > 0.0) {
        m_bubbleAlpha = qMax(0.0, m_bubbleAlpha - dt * 2.0);
    }

    if (m_mood == Mood::Sleeping) {
        m_headTilt = 12.0;
    } else if (m_mouseOver) {
        m_headTilt = qBound(-10.0, m_localMouse.x() * 0.15, 10.0);
    } else {
        m_headTilt *= 0.9;
    }
}

void DragonWidget::onAnimTick()
{
    const qreal dt = 0.033;
    m_time += dt;

    if (m_mood == Mood::Sleeping) {
        m_breathScale = 1.0 + 0.015 * qSin(m_time * 1.2);
        m_bounceY = 2.0 * qSin(m_time * 0.8);
        m_tailAngle = 4.0 * qSin(m_time * 1.5);
        m_wingAngle = 2.0 * qSin(m_time * 1.0);
        m_eyesClosed = true;
    } else {
        m_breathScale = 1.0 + 0.03 * qSin(m_time * 2.0);
        const qreal bounceAmp = (m_mood == Mood::Happy) ? 7.0 : 4.0;
        m_bounceY = bounceAmp * qSin(m_time * 1.8);
        const qreal tailAmp = (m_mood == Mood::Happy) ? 20.0 : 12.0;
        m_tailAngle = tailAmp * qSin(m_time * (m_mood == Mood::Happy ? 5.0 : 3.5));
        const qreal wingAmp = (m_mood == Mood::Eating) ? 8.0 : 18.0;
        m_wingAngle = wingAmp * qSin(m_time * 5.0);
    }

    if (m_breathingFire) {
        m_fireIntensity = qMin(1.0, m_fireIntensity + 0.08);
        if (m_time - m_fireStartTime > 1.5) {
            m_breathingFire = false;
        }
    } else {
        m_fireIntensity = qMax(0.0, m_fireIntensity - 0.06);
    }

    if (m_mouseOver && m_mood != Mood::Sleeping) {
        const qreal dx = qBound(-4.0, m_localMouse.x() * 0.12, 4.0);
        const qreal dy = qBound(-3.0, m_localMouse.y() * 0.08, 3.0);
        m_pupilX += (dx - m_pupilX) * 0.2;
        m_pupilY += (dy - m_pupilY) * 0.2;
    } else {
        m_pupilX *= 0.85;
        m_pupilY *= 0.85;
    }

    updateHearts(dt);
    updateFood(dt);
    updateFollowMouse(dt);
    updateMood(dt);

    update();
}

void DragonWidget::onBlink()
{
    if (m_mood == Mood::Sleeping) {
        scheduleNextBlink();
        return;
    }
    m_eyesClosed = true;
    update();
    QTimer::singleShot(120, this, [this]() {
        if (m_mood != Mood::Sleeping) {
            m_eyesClosed = false;
        }
        update();
        scheduleNextBlink();
    });
}

void DragonWidget::onIdleCheck()
{
    if (m_mouseOver || m_dragging || m_mood == Mood::Eating) {
        m_idleSeconds = 0.0;
        return;
    }
    m_idleSeconds += 1.0;
    if (m_idleSeconds >= 40.0 && m_mood != Mood::Sleeping) {
        setMood(Mood::Sleeping);
        showBubble(QStringLiteral("龙龙睡着了……"), 2000);
    }
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

    drawBubble(painter);

    painter.translate(width() / 2.0, height() / 2.0 + 10.0 + m_bounceY);
    painter.scale(m_breathScale, m_breathScale);

    drawDragon(painter);
    drawHearts(painter);
    drawFood(painter);

    if (m_mood == Mood::Sleeping) {
        drawSleepEffect(painter);
    }

    if (m_fireIntensity > 0.01) {
        drawFire(painter, m_fireIntensity);
    }
}

void DragonWidget::drawBubble(QPainter &painter)
{
    if (m_bubbleAlpha <= 0.01 || m_bubbleText.isEmpty()) {
        return;
    }

    painter.save();
    painter.setOpacity(m_bubbleAlpha);

    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    const QFontMetrics fm(font);
    const int textW = fm.horizontalAdvance(m_bubbleText);
    const int padH = 10;
    const int padV = 6;
    const QRect bubbleRect((width() - textW - padH * 2) / 2, 6,
                           textW + padH * 2, fm.height() + padV * 2);

    QPainterPath bubble;
    bubble.addRoundedRect(bubbleRect, 8, 8);
    bubble.moveTo(bubbleRect.center().x() - 6, bubbleRect.bottom());
    bubble.lineTo(bubbleRect.center().x(), bubbleRect.bottom() + 10);
    bubble.lineTo(bubbleRect.center().x() + 6, bubbleRect.bottom());
    bubble.closeSubpath();

    painter.setPen(QPen(QColor(60, 120, 110), 1.2));
    painter.setBrush(QColor(255, 255, 255, 230));
    painter.drawPath(bubble);

    painter.setPen(QColor(40, 80, 70));
    painter.drawText(bubbleRect, Qt::AlignCenter, m_bubbleText);
    painter.restore();
}

void DragonWidget::drawHearts(QPainter &painter)
{
    painter.save();
    for (const Heart &h : m_hearts) {
        painter.setOpacity(h.life);
        QPainterPath heart;
        const qreal x = h.pos.x();
        const qreal y = h.pos.y();
        const qreal size = 6.0 * h.life;
        heart.moveTo(x, y + size * 0.3);
        heart.cubicTo(x - size, y - size * 0.5, x - size * 0.2, y - size, x, y - size * 0.3);
        heart.cubicTo(x + size * 0.2, y - size, x + size, y - size * 0.5, x, y + size * 0.3);
        painter.setBrush(QColor(255, 105, 135));
        painter.setPen(Qt::NoPen);
        painter.drawPath(heart);
    }
    painter.restore();
}

void DragonWidget::drawFood(QPainter &painter)
{
    for (const Food &food : m_foods) {
        if (food.eaten) {
            continue;
        }
        painter.setBrush(QColor(255, 87, 51));
        painter.setPen(QPen(QColor(180, 50, 30), 1.0));
        painter.drawEllipse(food.pos, 10, 10);
        painter.setBrush(QColor(120, 200, 80));
        painter.drawEllipse(QPointF(food.pos.x() - 3, food.pos.y() - 8), 5, 4);
    }
}

void DragonWidget::drawSleepEffect(QPainter &painter)
{
    painter.save();
    QFont font = painter.font();
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(100, 160, 200, int(180 + 60 * qSin(m_time * 2.0))));
    painter.drawText(QPointF(20, -70), QStringLiteral("Z"));
    painter.drawText(QPointF(32, -82), QStringLiteral("z"));
    painter.drawText(QPointF(42, -92), QStringLiteral("z"));
    painter.restore();
}

void DragonWidget::drawDragon(QPainter &painter)
{
    const qreal s = 1.0;

    painter.save();
    painter.rotate(m_headTilt * 0.3);

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

    painter.setBrush(QColor(46, 139, 122));
    painter.setPen(QPen(QColor(30, 95, 85), 1.2));
    painter.drawEllipse(QRectF(-22 * s, 28 * s, 14 * s, 10 * s));
    painter.drawEllipse(QRectF(8 * s, 28 * s, 14 * s, 10 * s));

    QPainterPath neck;
    neck.addEllipse(QPointF(0, -18 * s), 22 * s, 20 * s);
    painter.setBrush(bodyGrad);
    painter.setPen(QPen(QColor(30, 95, 85), 1.5));
    painter.drawPath(neck);

    QPainterPath head;
    head.addEllipse(QPointF(0, -38 * s), 30 * s, 28 * s);
    painter.setBrush(bodyGrad);
    painter.drawPath(head);

    QPainterPath snout;
    snout.addEllipse(QPointF(0, -30 * s), 18 * s, 14 * s);
    painter.setBrush(QColor(90, 200, 175));
    painter.setPen(Qt::NoPen);
    painter.drawPath(snout);

    if (m_mood == Mood::Eating) {
        painter.setBrush(QColor(200, 80, 80));
        painter.drawEllipse(QRectF(-8 * s, -26 * s, 16 * s, 10 * s));
    } else {
        painter.setBrush(QColor(30, 80, 70));
        painter.drawEllipse(QRectF(-5 * s, -32 * s, 4 * s, 3 * s));
        painter.drawEllipse(QRectF(1 * s, -32 * s, 4 * s, 3 * s));
    }

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

    const qreal eyeY = -42 * s;
    const qreal eyeRX = 11 * s;
    const qreal eyeLX = -11 * s;
    if (m_eyesClosed || m_mood == Mood::Happy) {
        painter.setPen(QPen(QColor(30, 60, 55), 2.5, Qt::SolidLine, Qt::RoundCap));
        if (m_mood == Mood::Happy && !m_eyesClosed) {
            // 开心弯眼
            QPainterPath eyePathL;
            eyePathL.moveTo(eyeLX - 7 * s, eyeY);
            eyePathL.quadTo(eyeLX, eyeY - 5 * s, eyeLX + 7 * s, eyeY);
            QPainterPath eyePathR;
            eyePathR.moveTo(eyeRX - 7 * s, eyeY);
            eyePathR.quadTo(eyeRX, eyeY - 5 * s, eyeRX + 7 * s, eyeY);
            painter.drawPath(eyePathL);
            painter.drawPath(eyePathR);
        } else {
            painter.drawLine(QPointF(eyeLX - 7 * s, eyeY), QPointF(eyeLX + 7 * s, eyeY));
            painter.drawLine(QPointF(eyeRX - 7 * s, eyeY), QPointF(eyeRX + 7 * s, eyeY));
        }
    } else {
        painter.setBrush(Qt::white);
        painter.setPen(QPen(QColor(30, 60, 55), 1.2));
        painter.drawEllipse(QPointF(eyeLX, eyeY), 9 * s, 10 * s);
        painter.drawEllipse(QPointF(eyeRX, eyeY), 9 * s, 10 * s);

        const qreal pupilOffsetX = m_pupilX + 2.0 * qSin(m_time * 0.8);
        const qreal pupilOffsetY = m_pupilY + 1.0 * qCos(m_time * 0.6);
        painter.setBrush(QColor(25, 45, 40));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(eyeLX + pupilOffsetX, eyeY + pupilOffsetY), 4.5 * s, 5 * s);
        painter.drawEllipse(QPointF(eyeRX + pupilOffsetX, eyeY + pupilOffsetY), 4.5 * s, 5 * s);

        painter.setBrush(Qt::white);
        painter.drawEllipse(QPointF(eyeLX + pupilOffsetX - 2 * s, eyeY + pupilOffsetY - 2 * s), 2 * s, 2 * s);
        painter.drawEllipse(QPointF(eyeRX + pupilOffsetX - 2 * s, eyeY + pupilOffsetY - 2 * s), 2 * s, 2 * s);
    }

    if (m_blushAlpha > 0.01) {
        painter.setOpacity(m_blushAlpha * 0.6);
        painter.setBrush(QColor(255, 140, 160));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(-22 * s, -36 * s), 8 * s, 5 * s);
        painter.drawEllipse(QPointF(22 * s, -36 * s), 8 * s, 5 * s);
        painter.setOpacity(1.0);
    }

    painter.setBrush(QColor(56, 168, 148));
    painter.setPen(QPen(QColor(30, 95, 85), 1.2));
    painter.drawEllipse(QRectF(-28 * s, 10 * s, 12 * s, 10 * s));
    painter.drawEllipse(QRectF(16 * s, 10 * s, 12 * s, 10 * s));

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

void DragonWidget::enterEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_mouseOver = true;
    wakeUp();
    m_idleSeconds = 0.0;
}

void DragonWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_mouseOver = false;
}

void DragonWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_pressPos = event->pos();
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        wakeUp();
        event->accept();
    }
}

void DragonWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_localMouse = toDragonLocal(event->pos());

    if (event->buttons() & Qt::LeftButton) {
        const int dist = (event->pos() - m_pressPos).manhattanLength();
        if (dist > 6) {
            m_dragging = true;
        }
        if (m_dragging) {
            move(event->globalPos() - m_dragOffset);
        }
        event->accept();
    }
}

void DragonWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const int dist = (event->pos() - m_pressPos).manhattanLength();
        if (!m_dragging && dist <= 6) {
            const HitRegion region = hitTest(toDragonLocal(event->pos()));
            if (region != HitRegion::None) {
                triggerInteraction(region);
            }
        }
        m_dragging = false;
        event->accept();
    }
}

void DragonWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        wakeUp();
        m_breathingFire = true;
        m_fireIntensity = 0.0;
        m_fireStartTime = m_time;
        showBubble(QStringLiteral("呼——！"));
        event->accept();
    }
}

void DragonWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *feedAction = menu.addAction(QStringLiteral("喂食"));
    QAction *talkAction = menu.addAction(QStringLiteral("说句话"));
    QAction *followAction = menu.addAction(QStringLiteral("跟随鼠标"));
    followAction->setCheckable(true);
    followAction->setChecked(m_followMouse);
    menu.addSeparator();
    QAction *quitAction = menu.addAction(QStringLiteral("退出"));

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == feedAction) {
        feedDragon();
    } else if (chosen == talkAction) {
        sayRandomLine();
    } else if (chosen == followAction) {
        m_followMouse = followAction->isChecked();
        showBubble(m_followMouse ? QStringLiteral("我会跟着你~")
                                 : QStringLiteral("我就在这里等你"));
    } else if (chosen == quitAction) {
        qApp->quit();
    }
}
