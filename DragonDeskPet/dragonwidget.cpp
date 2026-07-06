#include "dragonwidget.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QCursor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLineF>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QRadialGradient>
#include <QtMath>

namespace {

const char *kRandomLines[] = {
    "行云布雨，护主安宁。",
    "吾乃东方青龙，在此镇守。",
    "摸龙须，福运自来。",
    "潜龙在渊，待时而飞。",
    "祥云环绕，紫气东来。",
    "陪你共度此间光阴。",
    "呼——云起风来。",
    "龙尾莫拽，否则翻江倒海。"
};

QColor scaleGreen(qreal shade)
{
    return QColor(int(20 + shade * 30), int(100 + shade * 80), int(80 + shade * 60));
}

} // namespace

DragonWidget::DragonWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(300, 290);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    for (int i = 0; i < 10; ++i) {
        CloudPuff cloud;
        cloud.pos = QPointF(-100.0 + (qrand() % 200), -70.0 + (qrand() % 150));
        cloud.size = 10.0 + (qrand() % 20);
        cloud.alpha = 0.12 + (qrand() % 25) / 100.0;
        cloud.drift = 0.3 + (qrand() % 8) / 10.0;
        m_clouds.append(cloud);
    }

    connect(&m_animTimer, &QTimer::timeout, this, &DragonWidget::onAnimTick);
    m_animTimer.start(33);

    connect(&m_blinkTimer, &QTimer::timeout, this, &DragonWidget::onBlink);
    scheduleNextBlink();

    connect(&m_idleTimer, &QTimer::timeout, this, &DragonWidget::onIdleCheck);
    m_idleTimer.start(1000);

    showBubble(QStringLiteral("青龙现世，祥瑞相随。"), 3000);
}

QPainterPath DragonWidget::buildSpinePath() const
{
    const qreal amp = (m_mood == Mood::Happy) ? 14.0
                      : (m_mood == Mood::Sleeping ? 3.0 : 9.0);
    const qreal wave = amp * qSin(m_time * 2.0);
    const qreal wave2 = (amp * 0.6) * qSin(m_time * 2.8 + 1.0);

    QPainterPath path;
    path.moveTo(58, -58);
    path.cubicTo(28 + wave * 0.2, -42, -18 - wave * 0.4, -18, -50 + wave, 4);
    path.cubicTo(-28 - wave2 * 0.3, 30, 22 + wave * 0.5, 52, 48 + m_tailWave, 78);
    return path;
}

QPointF DragonWidget::toDragonLocal(const QPoint &widgetPos) const
{
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0 + 5.0 + m_bounceY;
    const qreal x = (widgetPos.x() - cx) / m_breathScale;
    const qreal y = (widgetPos.y() - cy) / m_breathScale;
    return QPointF(x, y);
}

DragonWidget::HitRegion DragonWidget::hitTest(const QPointF &local) const
{
    const QPainterPath spine = buildSpinePath();
    qreal bestDist = 999.0;
    qreal bestT = 0.5;

    for (qreal t = 0.0; t <= 1.0; t += 0.04) {
        const QPointF pt = spine.pointAtPercent(t);
        const qreal dist = QLineF(local, pt).length();
        if (dist < bestDist) {
            bestDist = dist;
            bestT = t;
        }
    }

    if (bestDist > 28.0) {
        return HitRegion::None;
    }
    if (bestT < 0.15) {
        return HitRegion::Head;
    }
    if (bestT > 0.82) {
        return HitRegion::Tail;
    }
    return HitRegion::Body;
}

void DragonWidget::setMood(Mood mood)
{
    m_mood = mood;
    if (mood == Mood::Happy) {
        m_happyTimer = 2.5;
        m_glowAlpha = 1.0;
    } else if (mood == Mood::Normal) {
        m_glowAlpha = 0.0;
    }
}

void DragonWidget::wakeUp()
{
    if (m_mood == Mood::Sleeping) {
        setMood(Mood::Happy);
        showBubble(QStringLiteral("何人扰我清梦？"), 2000);
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
        h.pos = QPointF(20.0 + (qrand() % 40), -70.0 + (qrand() % 25));
        h.life = 1.0;
        h.vy = -30.0 - (qrand() % 15);
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
        showBubble(QStringLiteral("龙首不可轻触……罢了，许你摸之。"));
        m_headTilt = (m_localMouse.x() > 0 ? 6.0 : -6.0);
        break;
    case HitRegion::Body:
        setMood(Mood::Happy);
        spawnHearts(3);
        showBubble(QStringLiteral("龙鳞温润，祥云自生。"));
        break;
    case HitRegion::Tail:
        setMood(Mood::Happy);
        m_tailWave = 12.0;
        showBubble(QStringLiteral("龙尾轻摆，风雷响应。"));
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
    food.pos = QPointF(58, -78);
    food.vy = 0.0;
    m_foods.append(food);
    showBubble(QStringLiteral("含珠吞云，多谢投喂。"), 2000);
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
        h.life -= dt * 0.7;
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
        food.vy += 160.0 * dt;
        food.pos.setY(food.pos.y() + food.vy * dt);
        if (food.pos.y() >= -52.0) {
            food.eaten = true;
        }
    }
}

void DragonWidget::updateClouds(qreal dt)
{
    for (CloudPuff &cloud : m_clouds) {
        cloud.pos.setX(cloud.pos.x() + cloud.drift * dt * 8.0);
        cloud.pos.setY(cloud.pos.y() - cloud.drift * dt * 3.0);
        cloud.alpha = 0.1 + 0.08 * (1.0 + qSin(m_time * 1.5 + cloud.drift * 3.0));
        if (cloud.pos.x() > 130.0) {
            cloud.pos.setX(-130.0);
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

    if (m_glowAlpha > 0.0 && m_mood != Mood::Happy) {
        m_glowAlpha = qMax(0.0, m_glowAlpha - dt * 1.5);
    }

    if (m_bubbleTimer > 0.0) {
        m_bubbleTimer -= dt;
    } else if (m_bubbleAlpha > 0.0) {
        m_bubbleAlpha = qMax(0.0, m_bubbleAlpha - dt * 2.0);
    }

    m_tailWave *= 0.92;

    if (m_mood == Mood::Sleeping) {
        m_headTilt = 8.0;
    } else if (m_mouseOver) {
        m_headTilt = qBound(-8.0, m_localMouse.x() * 0.08, 8.0);
    } else {
        m_headTilt *= 0.9;
    }
}

void DragonWidget::onAnimTick()
{
    const qreal dt = 0.033;
    m_time += dt;

    if (m_mood == Mood::Sleeping) {
        m_breathScale = 1.0 + 0.01 * qSin(m_time * 1.0);
        m_bounceY = 1.5 * qSin(m_time * 0.6);
        m_bodyWave = 2.0 * qSin(m_time * 1.2);
        m_eyesClosed = true;
    } else {
        m_breathScale = 1.0 + 0.02 * qSin(m_time * 1.8);
        const qreal bounceAmp = (m_mood == Mood::Happy) ? 5.0 : 3.0;
        m_bounceY = bounceAmp * qSin(m_time * 1.5);
        m_bodyWave = (m_mood == Mood::Happy ? 16.0 : 10.0) * qSin(m_time * 2.2);
    }

    if (m_breathingFire) {
        m_fireIntensity = qMin(1.0, m_fireIntensity + 0.07);
        if (m_time - m_fireStartTime > 1.8) {
            m_breathingFire = false;
        }
    } else {
        m_fireIntensity = qMax(0.0, m_fireIntensity - 0.05);
    }

    if (m_mouseOver && m_mood != Mood::Sleeping) {
        const qreal dx = qBound(-3.0, m_localMouse.x() * 0.06, 3.0);
        const qreal dy = qBound(-2.0, m_localMouse.y() * 0.05, 2.0);
        m_pupilX += (dx - m_pupilX) * 0.15;
        m_pupilY += (dy - m_pupilY) * 0.15;
    } else {
        m_pupilX *= 0.85;
        m_pupilY *= 0.85;
    }

    updateHearts(dt);
    updateFood(dt);
    updateClouds(dt);
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
    QTimer::singleShot(150, this, [this]() {
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
        showBubble(QStringLiteral("龙入云眠……"), 2000);
    }
}

void DragonWidget::scheduleNextBlink()
{
    m_blinkTimer.start(3000 + qrand() % 4000);
}

void DragonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    drawBubble(painter);

    painter.translate(width() / 2.0, height() / 2.0 + 5.0 + m_bounceY);
    painter.scale(m_breathScale, m_breathScale);

    drawClouds(painter);
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

void DragonWidget::drawClouds(QPainter &painter)
{
    painter.save();
    for (const CloudPuff &cloud : m_clouds) {
        QRadialGradient grad(cloud.pos, cloud.size);
        grad.setColorAt(0.0, QColor(255, 255, 255, int(180 * cloud.alpha)));
        grad.setColorAt(0.5, QColor(230, 245, 255, int(100 * cloud.alpha)));
        grad.setColorAt(1.0, QColor(200, 230, 255, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(grad);
        painter.drawEllipse(cloud.pos, cloud.size * 1.2, cloud.size * 0.7);
        painter.drawEllipse(cloud.pos + QPointF(cloud.size * 0.5, cloud.size * 0.1),
                            cloud.size * 0.8, cloud.size * 0.5);
    }
    painter.restore();
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
    const QRect bubbleRect((width() - textW - padH * 2) / 2, 4,
                           textW + padH * 2, fm.height() + padV * 2);

    QPainterPath bubble;
    bubble.addRoundedRect(bubbleRect, 8, 8);
    bubble.moveTo(bubbleRect.center().x() - 6, bubbleRect.bottom());
    bubble.lineTo(bubbleRect.center().x(), bubbleRect.bottom() + 10);
    bubble.lineTo(bubbleRect.center().x() + 6, bubbleRect.bottom());
    bubble.closeSubpath();

    painter.setPen(QPen(QColor(40, 100, 80), 1.2));
    painter.setBrush(QColor(255, 255, 255, 235));
    painter.drawPath(bubble);

    painter.setPen(QColor(30, 70, 55));
    painter.drawText(bubbleRect, Qt::AlignCenter, m_bubbleText);
    painter.restore();
}

void DragonWidget::drawHearts(QPainter &painter)
{
    painter.save();
    for (const Heart &h : m_hearts) {
        painter.setOpacity(h.life);
        QPainterPath star;
        const qreal x = h.pos.x();
        const qreal y = h.pos.y();
        const qreal r = 5.0 * h.life;
        for (int i = 0; i < 4; ++i) {
            const qreal angle = i * M_PI / 2.0;
            star.lineTo(x + r * qCos(angle), y + r * qSin(angle));
            star.lineTo(x + r * 0.3 * qCos(angle + M_PI / 4.0),
                        y + r * 0.3 * qSin(angle + M_PI / 4.0));
        }
        star.closeSubpath();
        painter.setBrush(QColor(255, 215, 80, int(220 * h.life)));
        painter.setPen(Qt::NoPen);
        painter.drawPath(star);
    }
    painter.restore();
}

void DragonWidget::drawFood(QPainter &painter)
{
    for (const Food &food : m_foods) {
        if (food.eaten) {
            continue;
        }
        QRadialGradient pearl(food.pos, 10);
        pearl.setColorAt(0.0, QColor(255, 250, 220));
        pearl.setColorAt(0.5, QColor(255, 220, 100));
        pearl.setColorAt(1.0, QColor(220, 180, 60));
        painter.setBrush(pearl);
        painter.setPen(QPen(QColor(200, 160, 50), 1.0));
        painter.drawEllipse(food.pos, 9, 9);
        painter.setBrush(QColor(255, 255, 255, 180));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(food.pos.x() - 3, food.pos.y() - 3), 3, 2);
    }
}

void DragonWidget::drawSleepEffect(QPainter &painter)
{
    painter.save();
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(180, 210, 230, int(160 + 50 * qSin(m_time * 1.8))));
    painter.drawText(QPointF(30, -75), QStringLiteral("云"));
    painter.drawText(QPointF(44, -88), QStringLiteral("云"));
    painter.drawText(QPointF(56, -98), QStringLiteral("云"));
    painter.restore();
}

void DragonWidget::drawDragon(QPainter &painter)
{
    const QPainterPath spine = buildSpinePath();

    // 龙身主体
    QPainterPathStroker stroker;
    stroker.setWidth(28.0);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    const QPainterPath bodyOutline = stroker.createStroke(spine);

    QLinearGradient bodyGrad(-60, -40, 70, 80);
    bodyGrad.setColorAt(0.0, QColor(15, 90, 70));
    bodyGrad.setColorAt(0.35, QColor(30, 140, 110));
    bodyGrad.setColorAt(0.65, QColor(45, 170, 135));
    bodyGrad.setColorAt(1.0, QColor(20, 100, 80));
    painter.setPen(QPen(QColor(10, 60, 48), 1.5));
    painter.setBrush(bodyGrad);
    painter.drawPath(bodyOutline);

    // 白腹
    QPainterPathStroker bellyStroker;
    bellyStroker.setWidth(14.0);
    bellyStroker.setCapStyle(Qt::RoundCap);
    bellyStroker.setJoinStyle(Qt::RoundJoin);
    QPainterPath bellySpine = spine;
    const QPainterPath bellyPath = bellyStroker.createStroke(bellySpine);
    QLinearGradient bellyGrad(0, 0, 0, 80);
    bellyGrad.setColorAt(0.0, QColor(255, 252, 235));
    bellyGrad.setColorAt(0.5, QColor(245, 235, 200));
    bellyGrad.setColorAt(1.0, QColor(230, 215, 170));
    painter.setPen(Qt::NoPen);
    painter.setBrush(bellyGrad);
    painter.drawPath(bellyPath);

    // 鳞片纹理
    painter.save();
    painter.setPen(QPen(QColor(20, 110, 85, 90), 1.0));
    for (qreal t = 0.08; t < 0.92; t += 0.06) {
        const QPointF pt = spine.pointAtPercent(t);
        const QPointF tan = spine.pointAtPercent(t + 0.01) - spine.pointAtPercent(qMax(0.0, t - 0.01));
        const qreal angle = qAtan2(tan.y(), tan.x()) * 180.0 / M_PI;
        painter.save();
        painter.translate(pt);
        painter.rotate(angle);
        painter.drawArc(QRectF(-7, -5, 14, 10), 30 * 16, 120 * 16);
        painter.restore();
    }
    painter.restore();

    // 背脊鬃毛
    painter.save();
    for (qreal t = 0.05; t < 0.88; t += 0.07) {
        const QPointF pt = spine.pointAtPercent(t);
        const QPointF tan = spine.pointAtPercent(qMin(1.0, t + 0.02)) - spine.pointAtPercent(qMax(0.0, t - 0.02));
        const qreal angle = qAtan2(tan.y(), tan.x());
        const QPointF normal(-qSin(angle), qCos(angle));
        const qreal maneWave = 4.0 * qSin(m_time * 3.0 + t * 12.0);
        const QPointF tip = pt + normal * (-14.0 + maneWave);

        QLinearGradient maneGrad(pt, tip);
        maneGrad.setColorAt(0.0, QColor(40, 160, 120));
        maneGrad.setColorAt(1.0, QColor(80, 200, 160, 180));
        QPainterPath mane;
        mane.moveTo(pt + normal * (-4));
        mane.lineTo(tip);
        mane.lineTo(pt + QPointF(qCos(angle), qSin(angle)) * 5 + normal * (-2));
        mane.closeSubpath();
        painter.setBrush(maneGrad);
        painter.setPen(Qt::NoPen);
        painter.drawPath(mane);
    }
    painter.restore();

    // 龙爪
    const QVector<qreal> clawPositions = {0.22, 0.42, 0.62};
    for (qreal t : clawPositions) {
        const QPointF pt = spine.pointAtPercent(t);
        const QPointF tan = spine.pointAtPercent(qMin(1.0, t + 0.02)) - pt;
        const qreal angle = qAtan2(tan.y(), tan.x()) * 180.0 / M_PI;
        painter.save();
        painter.translate(pt);
        painter.rotate(angle + 90);
        painter.setBrush(QColor(25, 100, 78));
        painter.setPen(QPen(QColor(15, 60, 48), 1.0));
        for (int c = -1; c <= 1; c += 2) {
            painter.drawEllipse(QRectF(c * 10 - 3, 8, 6, 10));
            painter.drawLine(QPointF(c * 10, 16), QPointF(c * 12, 22));
            painter.drawLine(QPointF(c * 10 + 2, 16), QPointF(c * 14, 21));
        }
        painter.restore();
    }

    // 尾鳍
    const QPointF tailTip = spine.pointAtPercent(1.0);
    const QPointF tailTan = tailTip - spine.pointAtPercent(0.92);
    const qreal tailAngle = qAtan2(tailTan.y(), tailTan.x()) * 180.0 / M_PI;
    painter.save();
    painter.translate(tailTip);
    painter.rotate(tailAngle + m_tailWave);
    QPainterPath tailFin;
    tailFin.moveTo(0, 0);
    tailFin.cubicTo(-8, 12, -18, 18, -24, 8);
    tailFin.cubicTo(-16, 2, -8, -2, 0, 0);
    painter.setBrush(QColor(35, 150, 115));
    painter.setPen(QPen(QColor(15, 70, 55), 1.0));
    painter.drawPath(tailFin);
    painter.restore();

    // 龙头
    const QPointF headPos = spine.pointAtPercent(0.0);
    const QPointF headTan = spine.pointAtPercent(0.04) - headPos;
    const qreal headAngle = qAtan2(headTan.y(), headTan.x()) * 180.0 / M_PI + m_headTilt;
    drawDragonHead(painter, headPos, headAngle);

    // 祥光
    if (m_glowAlpha > 0.01) {
        painter.save();
        painter.setOpacity(m_glowAlpha * 0.35);
        QRadialGradient glow(headPos, 50);
        glow.setColorAt(0.0, QColor(120, 255, 180, 120));
        glow.setColorAt(1.0, QColor(120, 255, 180, 0));
        painter.setBrush(glow);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(headPos, 50, 50);
        painter.restore();
    }
}

void DragonWidget::drawDragonHead(QPainter &painter, const QPointF &headPos, qreal headAngle)
{
    painter.save();
    painter.translate(headPos);
    painter.rotate(headAngle);

    // 头型
    QPainterPath skull;
    skull.moveTo(8, 0);
    skull.cubicTo(18, -8, 28, -6, 32, 2);
    skull.cubicTo(34, 10, 26, 16, 14, 14);
    skull.cubicTo(4, 12, -2, 6, 8, 0);
    QLinearGradient headGrad(0, -10, 0, 16);
    headGrad.setColorAt(0.0, QColor(50, 175, 140));
    headGrad.setColorAt(0.5, QColor(35, 140, 110));
    headGrad.setColorAt(1.0, QColor(25, 100, 80));
    painter.setBrush(headGrad);
    painter.setPen(QPen(QColor(12, 55, 42), 1.5));
    painter.drawPath(skull);

    // 吻部
    QPainterPath snout;
    snout.moveTo(28, 2);
    snout.cubicTo(38, 0, 44, 4, 42, 8);
    snout.cubicTo(38, 12, 30, 10, 28, 2);
    painter.setBrush(QColor(60, 185, 150));
    painter.setPen(Qt::NoPen);
    painter.drawPath(snout);

    // 嘴
    if (m_mood == Mood::Eating || m_breathingFire) {
        painter.setBrush(QColor(180, 50, 40));
        painter.drawEllipse(QRectF(30, 2, 12, 8));
        painter.setPen(QColor(240, 240, 230));
        painter.drawLine(QPointF(34, 4), QPointF(34, 8));
        painter.drawLine(QPointF(38, 4), QPointF(38, 8));
    } else {
        painter.setPen(QPen(QColor(15, 50, 40), 1.5));
        painter.drawArc(QRectF(28, 4, 14, 8), 200 * 16, 100 * 16);
    }

    // 须
    painter.setPen(QPen(QColor(220, 245, 235), 1.2, Qt::SolidLine, Qt::RoundCap));
    const qreal whiskerWave = 3.0 * qSin(m_time * 2.5);
    for (int i = -1; i <= 1; ++i) {
        const qreal baseY = 4.0 + i * 3.0;
        QPainterPath whisker;
        whisker.moveTo(36, baseY);
        whisker.cubicTo(48, baseY - 4 + whiskerWave * i,
                        58, baseY + 2 + whiskerWave,
                        68 + whiskerWave, baseY + i * 4);
        painter.drawPath(whisker);
    }

    // 额下胡须
    painter.drawLine(QPointF(18, 12), QPointF(8, 22 + whiskerWave));
    painter.drawLine(QPointF(22, 13), QPointF(16, 24 - whiskerWave));

    // 鹿角
    painter.setPen(QPen(QColor(20, 35, 30), 1.5));
    painter.setBrush(QColor(30, 50, 42));
    auto drawAntler = [&](qreal sx, qreal sy, qreal dir) {
        QPainterPath antler;
        antler.moveTo(sx, sy);
        antler.lineTo(sx + dir * 6, sy - 18);
        antler.lineTo(sx + dir * 3, sy - 10);
        antler.lineTo(sx + dir * 12, sy - 24);
        antler.lineTo(sx + dir * 5, sy - 8);
        painter.drawPath(antler);
    };
    drawAntler(8, -4, -1);
    drawAntler(18, -6, 1);

    // 头鬃
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < 5; ++i) {
        const qreal fx = 4 + i * 5;
        const qreal fw = 2.0 * qSin(m_time * 3.0 + i);
        QPainterPath mane;
        mane.moveTo(fx, -6);
        mane.cubicTo(fx - 4 + fw, -16, fx + 2, -22 + fw, fx + 6, -14);
        mane.cubicTo(fx + 2, -10, fx, -4, fx, -6);
        QLinearGradient mg(fx, -6, fx, -20);
        mg.setColorAt(0.0, QColor(45, 160, 125));
        mg.setColorAt(1.0, QColor(90, 210, 170, 200));
        painter.setBrush(mg);
        painter.drawPath(mane);
    }

    // 眼
    const QPointF eyePos(22, 0);
    if (m_eyesClosed || m_mood == Mood::Sleeping) {
        painter.setPen(QPen(QColor(15, 40, 30), 2.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(18, 0), QPointF(28, 0));
    } else {
        painter.setBrush(QColor(10, 30, 22));
        painter.setPen(QPen(QColor(8, 25, 18), 1.0));
        painter.drawEllipse(eyePos, 7, 8);

        const qreal glow = 0.6 + 0.4 * qSin(m_time * 3.0);
        painter.setBrush(QColor(int(80 + 60 * glow), 255, int(120 + 80 * glow)));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(eyePos.x() + m_pupilX, eyePos.y() + m_pupilY), 4, 5);

        painter.setBrush(QColor(200, 255, 220, int(180 * glow)));
        painter.drawEllipse(QPointF(eyePos.x() + m_pupilX - 1.5, eyePos.y() + m_pupilY - 2), 2, 2);

        // 眼辉
        painter.setOpacity(0.4 * glow);
        QRadialGradient eyeGlow(eyePos, 14);
        eyeGlow.setColorAt(0.0, QColor(100, 255, 150, 100));
        eyeGlow.setColorAt(1.0, QColor(100, 255, 150, 0));
        painter.setBrush(eyeGlow);
        painter.drawEllipse(eyePos, 14, 14);
        painter.setOpacity(1.0);
    }

    painter.restore();
}

void DragonWidget::drawFire(QPainter &painter, qreal intensity)
{
    const QPainterPath spine = buildSpinePath();
    const QPointF headPos = spine.pointAtPercent(0.0);
    const QPointF headTan = spine.pointAtPercent(0.04) - headPos;
    const qreal headAngle = qAtan2(headTan.y(), headTan.x());

    painter.save();
    painter.translate(headPos);
    painter.rotate(headAngle * 180.0 / M_PI + m_headTilt);

    // 云气
    for (int i = 0; i < 4; ++i) {
        const qreal spread = (i - 1.5) * 10.0;
        const qreal puff = qSin(m_time * 4.0 + i) * 5.0;
        QRadialGradient cloud(spread, -8 + puff, 18 * intensity);
        cloud.setColorAt(0.0, QColor(220, 255, 240, int(160 * intensity)));
        cloud.setColorAt(0.6, QColor(150, 230, 200, int(80 * intensity)));
        cloud.setColorAt(1.0, QColor(150, 230, 200, 0));
        painter.setBrush(cloud);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(spread + 20, -6 + puff), 16 * intensity, 12 * intensity);
    }

    // 龙焰
    for (int i = 0; i < 5; ++i) {
        const qreal spread = (i - 2) * 7.0;
        const qreal flicker = qSin(m_time * 10.0 + i) * 3.0;
        const qreal flameH = (22.0 + flicker) * intensity;
        const qreal flameW = 8.0 * intensity;

        QPainterPath flame;
        flame.moveTo(38 + spread - flameW * 0.5, 4);
        flame.cubicTo(38 + spread - flameW, 4 - flameH * 0.4,
                      38 + spread, 4 - flameH,
                      38 + spread + flameW * 0.3, 4 - flameH * 1.1);
        flame.cubicTo(38 + spread + flameW, 4 - flameH * 0.5,
                      38 + spread + flameW * 0.5, 4,
                      38 + spread + flameW * 0.5, 4);

        QLinearGradient grad(38 + spread, 4, 38 + spread, 4 - flameH);
        grad.setColorAt(0.0, QColor(255, 240, 120, int(220 * intensity)));
        grad.setColorAt(0.4, QColor(255, 160, 40, int(200 * intensity)));
        grad.setColorAt(1.0, QColor(255, 60, 30, int(60 * intensity)));
        painter.setBrush(grad);
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
        showBubble(QStringLiteral("龙吟九天，云火齐发！"));
        event->accept();
    }
}

void DragonWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *feedAction = menu.addAction(QStringLiteral("投喂灵珠"));
    QAction *talkAction = menu.addAction(QStringLiteral("听龙语"));
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
        showBubble(m_followMouse ? QStringLiteral("祥云引路，随你而行。")
                                 : QStringLiteral("守此一方，静候召唤。"));
    } else if (chosen == quitAction) {
        qApp->quit();
    }
}
