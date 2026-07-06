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
    "天地之间有正气，龙威浩荡镇八方。",
    "吾居云端，俯瞰众生。",
    "风云际会，神龙在天。",
    "触龙鳞者，当知天威不可犯。",
    "山河万里，唯龙独尊。",
    "紫气东来，万邪退散。",
    "一声龙吟，四海皆惊。",
    "云从龙，风从虎，势不可挡。"
};

void drawCloudBlob(QPainter &p, const QPointF &c, qreal w, qreal h, qreal alpha)
{
    QRadialGradient g(c, qMax(w, h));
    g.setColorAt(0.0, QColor(255, 255, 255, int(200 * alpha)));
    g.setColorAt(0.45, QColor(235, 242, 250, int(120 * alpha)));
    g.setColorAt(1.0, QColor(210, 225, 240, 0));
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, w, h);
    p.drawEllipse(c + QPointF(w * 0.45, h * 0.05), w * 0.65, h * 0.55);
    p.drawEllipse(c + QPointF(-w * 0.35, h * 0.12), w * 0.5, h * 0.45);
}

} // namespace

DragonWidget::DragonWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(360, 340);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    for (int i = 0; i < 18; ++i) {
        CloudPuff cloud;
        cloud.pos = QPointF(-140.0 + (qrand() % 280), -100.0 + (qrand() % 200));
        cloud.size = 16.0 + (qrand() % 35);
        cloud.alpha = 0.18 + (qrand() % 35) / 100.0;
        cloud.drift = 0.2 + (qrand() % 6) / 10.0;
        cloud.wrapsBody = (i % 3 == 0);
        if (cloud.wrapsBody) {
            cloud.bodyT = 0.1 + (qrand() % 80) / 100.0;
        }
        m_clouds.append(cloud);
    }

    connect(&m_animTimer, &QTimer::timeout, this, &DragonWidget::onAnimTick);
    m_animTimer.start(33);

    connect(&m_blinkTimer, &QTimer::timeout, this, &DragonWidget::onBlink);
    scheduleNextBlink();

    connect(&m_idleTimer, &QTimer::timeout, this, &DragonWidget::onIdleCheck);
    m_idleTimer.start(1000);

    m_lastGlobalPos = pos();
    m_auraAlpha = 0.35;
    showBubble(QStringLiteral("神龙降世，云雾随行。"), 3000);
}

QPainterPath DragonWidget::buildSpinePath() const
{
    const qreal amp = (m_mood == Mood::Majestic) ? 11.0
                      : (m_mood == Mood::Sleeping ? 2.0 : 7.0);
    const qreal wave = amp * qSin(m_time * 1.4);
    const qreal wave2 = (amp * 0.5) * qSin(m_time * 1.9 + 0.8);

    QPainterPath path;
    path.moveTo(72, -68);
    path.cubicTo(38 + wave * 0.15, -48, -28 - wave * 0.35, -22, -62 + wave, 6);
    path.cubicTo(-38 - wave2 * 0.25, 38, 28 + wave * 0.4, 62, 58 + m_tailWave, 92);
    return path;
}

QPointF DragonWidget::toDragonLocal(const QPoint &widgetPos) const
{
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0 + 8.0 + m_bounceY;
    return QPointF((widgetPos.x() - cx) / m_breathScale,
                   (widgetPos.y() - cy) / m_breathScale);
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

    if (bestDist > 32.0) {
        return HitRegion::None;
    }
    if (bestT < 0.14) {
        return HitRegion::Head;
    }
    if (bestT > 0.84) {
        return HitRegion::Tail;
    }
    return HitRegion::Body;
}

void DragonWidget::setMood(Mood mood)
{
    m_mood = mood;
    if (mood == Mood::Majestic) {
        m_majesticTimer = 2.8;
        m_auraAlpha = 1.0;
    } else if (mood == Mood::Normal) {
        m_auraAlpha = 0.35;
    }
}

void DragonWidget::wakeUp()
{
    if (m_mood == Mood::Sleeping) {
        setMood(Mood::Majestic);
        showBubble(QStringLiteral("何人惊扰神龙？"), 2200);
        m_idleSeconds = 0.0;
        emitBodyMist(8);
    }
}

void DragonWidget::showBubble(const QString &text, int durationMs)
{
    m_bubbleText = text;
    m_bubbleAlpha = 1.0;
    m_bubbleTimer = durationMs / 1000.0;
}

void DragonWidget::spawnAuraPulse(const QPointF &center)
{
    AuraRing ring;
    ring.center = center;
    ring.radius = 8.0;
    ring.maxRadius = 55.0;
    ring.life = 1.0;
    m_auras.append(ring);
}

void DragonWidget::emitBodyMist(int count)
{
    if (m_mists.size() > 120) {
        return;
    }
    const QPainterPath spine = buildSpinePath();
    for (int i = 0; i < count; ++i) {
        const qreal t = 0.05 + (qrand() % 90) / 100.0;
        const QPointF pt = spine.pointAtPercent(t);
        MistParticle m;
        m.pos = pt + QPointF((qrand() % 20) - 10, (qrand() % 16) - 8);
        m.vel = QPointF((qrand() % 10) - 5, -8.0 - (qrand() % 12));
        m.size = 10.0 + (qrand() % 18);
        m.life = 1.0;
        m.alpha = 0.35 + (qrand() % 30) / 100.0;
        m_mists.append(m);
    }
}

void DragonWidget::emitDragMist()
{
    if (m_mists.size() > 120) {
        return;
    }
    const QPainterPath spine = buildSpinePath();
    for (qreal t = 0.2; t < 0.9; t += 0.25) {
        MistParticle m;
        m.pos = spine.pointAtPercent(t);
        m.vel = QPointF(-12.0 - (qrand() % 8), (qrand() % 6) - 3);
        m.size = 14.0 + (qrand() % 12);
        m.life = 0.9;
        m.alpha = 0.45;
        m_mists.append(m);
    }
}

void DragonWidget::triggerInteraction(HitRegion region)
{
    wakeUp();
    m_idleSeconds = 0.0;
    const QPainterPath spine = buildSpinePath();

    switch (region) {
    case HitRegion::Head:
        setMood(Mood::Majestic);
        spawnAuraPulse(spine.pointAtPercent(0.0));
        emitBodyMist(8);
        showBubble(QStringLiteral("触龙首者，当思其威。"));
        m_headTilt = (m_localMouse.x() > 0 ? 4.0 : -4.0);
        break;
    case HitRegion::Body:
        setMood(Mood::Majestic);
        spawnAuraPulse(spine.pointAtPercent(0.45));
        emitBodyMist(6);
        showBubble(QStringLiteral("龙躯不动，风云自生。"));
        break;
    case HitRegion::Tail:
        setMood(Mood::Majestic);
        m_tailWave = 18.0;
        spawnAuraPulse(spine.pointAtPercent(1.0));
        emitBodyMist(8);
        showBubble(QStringLiteral("撼龙尾者，天地为之色变！"));
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
    food.pos = QPointF(72, -88);
    food.vy = 0.0;
    m_foods.append(food);
    showBubble(QStringLiteral("吞珠纳云，威势更盛。"), 2200);
}

void DragonWidget::sayRandomLine()
{
    wakeUp();
    const int count = int(sizeof(kRandomLines) / sizeof(kRandomLines[0]));
    showBubble(QString::fromUtf8(kRandomLines[qrand() % count]));
}

void DragonWidget::updateAura(qreal dt)
{
    for (int i = m_auras.size() - 1; i >= 0; --i) {
        AuraRing &r = m_auras[i];
        r.radius += 45.0 * dt;
        r.life -= dt * 0.55;
        if (r.life <= 0.0 || r.radius > r.maxRadius) {
            m_auras.removeAt(i);
        }
    }
}

void DragonWidget::updateFood(qreal dt)
{
    for (Food &food : m_foods) {
        if (food.eaten) {
            continue;
        }
        food.vy += 140.0 * dt;
        food.pos.setY(food.pos.y() + food.vy * dt);
        if (food.pos.y() >= -58.0) {
            food.eaten = true;
            emitBodyMist(8);
        }
    }
}

void DragonWidget::updateClouds(qreal dt)
{
    const QPainterPath spine = buildSpinePath();
    for (CloudPuff &cloud : m_clouds) {
        if (cloud.wrapsBody && cloud.bodyT >= 0.0) {
            const QPointF bodyPt = spine.pointAtPercent(cloud.bodyT);
            const qreal wrap = 12.0 * qSin(m_time * 1.2 + cloud.bodyT * 8.0);
            cloud.pos = bodyPt + QPointF(wrap, 8.0 + wrap * 0.3);
            cloud.alpha = 0.22 + 0.12 * (1.0 + qSin(m_time * 1.8 + cloud.drift * 5.0));
        } else {
            cloud.pos.setX(cloud.pos.x() + cloud.drift * dt * 6.0);
            cloud.pos.setY(cloud.pos.y() - cloud.drift * dt * 2.5);
            cloud.alpha = 0.12 + 0.1 * (1.0 + qSin(m_time * 1.2 + cloud.drift * 4.0));
            if (cloud.pos.x() > 160.0) {
                cloud.pos.setX(-160.0);
            }
        }
    }
}

void DragonWidget::updateMist(qreal dt)
{
    for (int i = m_mists.size() - 1; i >= 0; --i) {
        MistParticle &m = m_mists[i];
        m.pos += m.vel * dt;
        m.vel.setX(m.vel.x() * 0.98);
        m.vel.setY(m.vel.y() - 6.0 * dt);
        m.size += 8.0 * dt;
        m.life -= dt * 0.45;
        m.alpha *= 0.992;
        if (m.life <= 0.0) {
            m_mists.removeAt(i);
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
    if (line.length() < 90.0) {
        return;
    }

    const qreal step = qMin(90.0 * dt, line.length() - 70.0);
    const qreal ratio = step / line.length();
    move(pos() + QPoint(int((cursor.x() - globalCenter.x()) * ratio),
                        int((cursor.y() - globalCenter.y()) * ratio)));
}

void DragonWidget::updateMood(qreal dt)
{
    if (m_majesticTimer > 0.0) {
        m_majesticTimer -= dt;
        if (m_majesticTimer <= 0.0 && m_mood == Mood::Majestic) {
            setMood(Mood::Normal);
        }
    }

    if (m_eatTimer > 0.0) {
        m_eatTimer -= dt;
        if (m_eatTimer <= 0.0 && m_mood == Mood::Eating) {
            setMood(Mood::Majestic);
            spawnAuraPulse(QPointF(0, 0));
        }
    }

    if (m_auraAlpha > 0.35 && m_mood != Mood::Majestic) {
        m_auraAlpha = qMax(0.35, m_auraAlpha - dt * 0.4);
    }

    if (m_bubbleTimer > 0.0) {
        m_bubbleTimer -= dt;
    } else if (m_bubbleAlpha > 0.0) {
        m_bubbleAlpha = qMax(0.0, m_bubbleAlpha - dt * 1.8);
    }

    m_tailWave *= 0.94;

    if (m_mood == Mood::Sleeping) {
        m_headTilt = 6.0;
    } else if (m_mouseOver) {
        m_headTilt = qBound(-6.0, m_localMouse.x() * 0.04, 6.0);
    } else {
        m_headTilt *= 0.92;
    }
}

void DragonWidget::onAnimTick()
{
    const qreal dt = 0.033;
    m_time += dt;

    if (m_mood == Mood::Sleeping) {
        m_breathScale = 1.0 + 0.008 * qSin(m_time * 0.8);
        m_bounceY = 1.0 * qSin(m_time * 0.5);
        m_bodyWave = 1.5 * qSin(m_time * 0.9);
        m_eyesClosed = true;
    } else {
        m_breathScale = 1.0 + 0.012 * qSin(m_time * 1.2);
        m_bounceY = 2.5 * qSin(m_time * 0.9);
        m_bodyWave = (m_mood == Mood::Majestic ? 9.0 : 6.0) * qSin(m_time * 1.6);
    }

    if (m_breathingFire) {
        m_fireIntensity = qMin(1.0, m_fireIntensity + 0.06);
        if (m_time - m_fireStartTime > 2.0) {
            m_breathingFire = false;
        }
    } else {
        m_fireIntensity = qMax(0.0, m_fireIntensity - 0.04);
    }

    if (m_mouseOver && m_mood != Mood::Sleeping) {
        const qreal dx = qBound(-2.5, m_localMouse.x() * 0.04, 2.5);
        const qreal dy = qBound(-1.5, m_localMouse.y() * 0.03, 1.5);
        m_pupilX += (dx - m_pupilX) * 0.12;
        m_pupilY += (dy - m_pupilY) * 0.12;
    } else {
        m_pupilX *= 0.9;
        m_pupilY *= 0.9;
    }

    m_mistEmitTimer += dt;
    if (m_mistEmitTimer > 0.18) {
        m_mistEmitTimer = 0.0;
        emitBodyMist(2);
    }

    if (m_dragging) {
        const QPoint gp = pos();
        if ((gp - m_lastGlobalPos).manhattanLength() > 8) {
            emitDragMist();
            m_lastGlobalPos = gp;
        }
    }

    updateAura(dt);
    updateFood(dt);
    updateClouds(dt);
    updateMist(dt);
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
    QTimer::singleShot(80, this, [this]() {
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
    if (m_idleSeconds >= 50.0 && m_mood != Mood::Sleeping) {
        setMood(Mood::Sleeping);
        showBubble(QStringLiteral("龙隐云深，静候天时。"), 2200);
    }
}

void DragonWidget::scheduleNextBlink()
{
    m_blinkTimer.start(5000 + qrand() % 6000);
}

void DragonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    drawBubble(painter);

    painter.translate(width() / 2.0, height() / 2.0 + 8.0 + m_bounceY);
    painter.scale(m_breathScale, m_breathScale);

    drawBackgroundMist(painter);
    drawBodyWrapMist(painter, true);
    drawDragonShadow(painter);
    drawDragon(painter);
    drawBodyWrapMist(painter, false);
    drawMistParticles(painter);
    drawAuraRings(painter);
    drawFood(painter);

    if (m_mood == Mood::Sleeping) {
        drawSleepEffect(painter);
    }

    if (m_fireIntensity > 0.01) {
        drawFire(painter, m_fireIntensity);
    }
}

void DragonWidget::drawBackgroundMist(QPainter &painter)
{
    painter.save();
    const QVector<QPointF> anchors = {
        QPointF(-90, 20), QPointF(60, -30), QPointF(-40, 70),
        QPointF(100, 40), QPointF(0, -80), QPointF(-110, -40)
    };
    for (int i = 0; i < anchors.size(); ++i) {
        const QPointF &a = anchors[i];
        const qreal pulse = 1.0 + 0.08 * qSin(m_time * 0.8 + i);
        drawCloudBlob(painter, a, 45 * pulse, 22 * pulse, 0.35 + 0.1 * qSin(m_time + i));
    }
    for (const CloudPuff &cloud : m_clouds) {
        if (!cloud.wrapsBody) {
            drawCloudBlob(painter, cloud.pos, cloud.size * 1.3, cloud.size * 0.65, cloud.alpha);
        }
    }
    painter.restore();
}

void DragonWidget::drawBodyWrapMist(QPainter &painter, bool behindDragon)
{
    painter.save();
    for (const CloudPuff &cloud : m_clouds) {
        if (!cloud.wrapsBody) {
            continue;
        }
        const bool front = cloud.bodyT > 0.45;
        if (behindDragon == front) {
            continue;
        }
        drawCloudBlob(painter, cloud.pos, cloud.size, cloud.size * 0.55,
                      cloud.alpha * (behindDragon ? 0.7 : 1.0));
    }
    painter.restore();
}

void DragonWidget::drawMistParticles(QPainter &painter)
{
    painter.save();
    for (const MistParticle &m : m_mists) {
        drawCloudBlob(painter, m.pos, m.size, m.size * 0.5, m.alpha * m.life);
    }
    painter.restore();
}

void DragonWidget::drawDragonShadow(QPainter &painter)
{
    const QPainterPath spine = buildSpinePath();
    QPainterPathStroker s;
    s.setWidth(34.0);
    s.setCapStyle(Qt::RoundCap);
    s.setJoinStyle(Qt::RoundJoin);
    QPainterPath shadow = s.createStroke(spine);
    painter.save();
    painter.translate(4, 6);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 30, 25, 45));
    painter.drawPath(shadow);
    painter.restore();
}

void DragonWidget::drawAuraRings(QPainter &painter)
{
    painter.save();
    for (const AuraRing &r : m_auras) {
        QPen pen(QColor(180, 220, 160, int(160 * r.life)));
        pen.setWidthF(2.0);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(r.center, r.radius, r.radius * 0.65);
    }

    if (m_auraAlpha > 0.01) {
        const QPainterPath spine = buildSpinePath();
        const QPointF head = spine.pointAtPercent(0.0);
        QRadialGradient aura(head, 70);
        aura.setColorAt(0.0, QColor(120, 200, 160, int(90 * m_auraAlpha)));
        aura.setColorAt(0.6, QColor(80, 160, 130, int(40 * m_auraAlpha)));
        aura.setColorAt(1.0, QColor(60, 120, 100, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(aura);
        painter.drawEllipse(head, 70, 55);
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
    font.setBold(true);
    painter.setFont(font);

    const QFontMetrics fm(font);
    const int textW = fm.horizontalAdvance(m_bubbleText);
    const int padH = 12;
    const int padV = 7;
    const QRect bubbleRect((width() - textW - padH * 2) / 2, 2,
                           textW + padH * 2, fm.height() + padV * 2);

    QLinearGradient bg(bubbleRect.topLeft(), bubbleRect.bottomRight());
    bg.setColorAt(0.0, QColor(18, 55, 45, 230));
    bg.setColorAt(1.0, QColor(8, 35, 28, 240));

    QPainterPath bubble;
    bubble.addRoundedRect(bubbleRect, 6, 6);

    painter.setPen(QPen(QColor(180, 210, 120), 1.5));
    painter.setBrush(bg);
    painter.drawPath(bubble);

    painter.setPen(QColor(230, 245, 210));
    painter.drawText(bubbleRect, Qt::AlignCenter, m_bubbleText);
    painter.restore();
}

void DragonWidget::drawFood(QPainter &painter)
{
    for (const Food &food : m_foods) {
        if (food.eaten) {
            continue;
        }
        QRadialGradient pearl(food.pos, 12);
        pearl.setColorAt(0.0, QColor(255, 255, 240));
        pearl.setColorAt(0.4, QColor(255, 230, 120));
        pearl.setColorAt(1.0, QColor(200, 160, 40));
        painter.setBrush(pearl);
        painter.setPen(QPen(QColor(160, 130, 30), 1.2));
        painter.drawEllipse(food.pos, 11, 11);
        painter.setBrush(QColor(255, 255, 255, 200));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(food.pos.x() - 3, food.pos.y() - 4), 4, 3);
    }
}

void DragonWidget::drawSleepEffect(QPainter &painter)
{
    painter.save();
    for (int i = 0; i < 5; ++i) {
        const qreal x = 20.0 + i * 18;
        const qreal y = -85.0 - i * 8 + 4.0 * qSin(m_time * 1.5 + i);
        drawCloudBlob(painter, QPointF(x, y), 14 + i * 2, 8, 0.25 + 0.1 * qSin(m_time + i));
    }
    painter.restore();
}

void DragonWidget::drawDragon(QPainter &painter)
{
    const QPainterPath spine = buildSpinePath();

    QPainterPathStroker stroker;
    stroker.setWidth(34.0);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    const QPainterPath bodyOutline = stroker.createStroke(spine);

    QLinearGradient bodyGrad(-80, -50, 90, 100);
    bodyGrad.setColorAt(0.0, QColor(8, 55, 42));
    bodyGrad.setColorAt(0.25, QColor(18, 95, 72));
    bodyGrad.setColorAt(0.55, QColor(28, 130, 98));
    bodyGrad.setColorAt(0.78, QColor(45, 165, 125));
    bodyGrad.setColorAt(1.0, QColor(12, 70, 55));
    painter.setPen(QPen(QColor(5, 35, 28), 2.0));
    painter.setBrush(bodyGrad);
    painter.drawPath(bodyOutline);

    // 鳞光高光
    painter.save();
    painter.setClipPath(bodyOutline);
    QLinearGradient sheen(-60, -80, 80, 60);
    sheen.setColorAt(0.0, QColor(255, 255, 255, 0));
    sheen.setColorAt(0.35, QColor(180, 255, 220, 35));
    sheen.setColorAt(0.5, QColor(255, 255, 255, 0));
    painter.fillRect(QRectF(-120, -120, 240, 240), sheen);
    painter.restore();

    QPainterPathStroker bellyStroker;
    bellyStroker.setWidth(16.0);
    bellyStroker.setCapStyle(Qt::RoundCap);
    bellyStroker.setJoinStyle(Qt::RoundJoin);
    const QPainterPath bellyPath = bellyStroker.createStroke(spine);
    QLinearGradient bellyGrad(0, -20, 0, 100);
    bellyGrad.setColorAt(0.0, QColor(252, 250, 238));
    bellyGrad.setColorAt(0.5, QColor(240, 230, 200));
    bellyGrad.setColorAt(1.0, QColor(220, 205, 165));
    painter.setPen(Qt::NoPen);
    painter.setBrush(bellyGrad);
    painter.drawPath(bellyPath);

    painter.save();
    painter.setPen(QPen(QColor(10, 70, 52, 100), 1.2));
    for (qreal t = 0.06; t < 0.94; t += 0.055) {
        const QPointF pt = spine.pointAtPercent(t);
        const QPointF tan = spine.pointAtPercent(qMin(1.0, t + 0.015))
                            - spine.pointAtPercent(qMax(0.0, t - 0.015));
        const qreal angle = qAtan2(tan.y(), tan.x()) * 180.0 / M_PI;
        painter.save();
        painter.translate(pt);
        painter.rotate(angle);
        painter.drawArc(QRectF(-8, -6, 16, 11), 20 * 16, 140 * 16);
        painter.restore();
    }
    painter.restore();

    // 背脊长鬃
    painter.save();
    for (qreal t = 0.04; t < 0.9; t += 0.06) {
        const QPointF pt = spine.pointAtPercent(t);
        const QPointF tan = spine.pointAtPercent(qMin(1.0, t + 0.02))
                            - spine.pointAtPercent(qMax(0.0, t - 0.02));
        const qreal angle = qAtan2(tan.y(), tan.x());
        const QPointF normal(-qSin(angle), qCos(angle));
        const qreal maneWave = 5.0 * qSin(m_time * 2.2 + t * 10.0);
        const QPointF tip = pt + normal * (-20.0 + maneWave);

        QLinearGradient maneGrad(pt, tip);
        maneGrad.setColorAt(0.0, QColor(25, 110, 85));
        maneGrad.setColorAt(0.5, QColor(40, 150, 115));
        maneGrad.setColorAt(1.0, QColor(70, 190, 150, 160));
        QPainterPath mane;
        mane.moveTo(pt + normal * (-5));
        mane.cubicTo(pt + normal * (-12) + QPointF(maneWave, 0), tip,
                     pt + QPointF(qCos(angle), qSin(angle)) * 6 + normal * (-3));
        mane.closeSubpath();
        painter.setBrush(maneGrad);
        painter.setPen(Qt::NoPen);
        painter.drawPath(mane);
    }
    painter.restore();

    const QVector<qreal> clawPositions = {0.2, 0.42, 0.64};
    for (qreal t : clawPositions) {
        const QPointF pt = spine.pointAtPercent(t);
        const QPointF tan = spine.pointAtPercent(qMin(1.0, t + 0.02)) - pt;
        const qreal angle = qAtan2(tan.y(), tan.x()) * 180.0 / M_PI;
        painter.save();
        painter.translate(pt);
        painter.rotate(angle + 90);
        painter.setBrush(QColor(15, 70, 55));
        painter.setPen(QPen(QColor(8, 40, 32), 1.2));
        for (int c = -1; c <= 1; c += 2) {
            painter.drawEllipse(QRectF(c * 12 - 4, 10, 8, 12));
            painter.drawLine(QPointF(c * 12, 20), QPointF(c * 15, 28));
            painter.drawLine(QPointF(c * 12 + 2, 20), QPointF(c * 17, 26));
            painter.drawLine(QPointF(c * 12 - 2, 20), QPointF(c * 13, 27));
        }
        painter.restore();
    }

    const QPointF tailTip = spine.pointAtPercent(1.0);
    const QPointF tailTan = tailTip - spine.pointAtPercent(0.9);
    const qreal tailAngle = qAtan2(tailTan.y(), tailTan.x()) * 180.0 / M_PI;
    painter.save();
    painter.translate(tailTip);
    painter.rotate(tailAngle + m_tailWave);
    QPainterPath tailFin;
    tailFin.moveTo(0, 0);
    tailFin.cubicTo(-10, 16, -24, 24, -32, 10);
    tailFin.cubicTo(-20, 0, -10, -4, 0, 0);
    painter.setBrush(QColor(20, 100, 78));
    painter.setPen(QPen(QColor(8, 45, 35), 1.2));
    painter.drawPath(tailFin);
    painter.restore();

    const QPointF headPos = spine.pointAtPercent(0.0);
    const QPointF headTan = spine.pointAtPercent(0.035) - headPos;
    const qreal headAngle = qAtan2(headTan.y(), headTan.x()) * 180.0 / M_PI + m_headTilt;
    drawDragonHead(painter, headPos, headAngle);
}

void DragonWidget::drawDragonHead(QPainter &painter, const QPointF &headPos, qreal headAngle)
{
    painter.save();
    painter.translate(headPos);
    painter.rotate(headAngle);
    painter.rotate(-38.0);

    QPainterPath skull;
    skull.moveTo(6, 2);
    skull.cubicTo(20, -10, 36, -8, 42, 4);
    skull.cubicTo(46, 14, 36, 22, 20, 20);
    skull.cubicTo(8, 18, 0, 10, 6, 2);
    QLinearGradient headGrad(0, -12, 0, 22);
    headGrad.setColorAt(0.0, QColor(35, 145, 110));
    headGrad.setColorAt(0.5, QColor(22, 100, 78));
    headGrad.setColorAt(1.0, QColor(12, 65, 50));
    painter.setBrush(headGrad);
    painter.setPen(QPen(QColor(5, 30, 22), 2.0));
    painter.drawPath(skull);

    QPainterPath snout;
    snout.moveTo(34, 4);
    snout.cubicTo(50, 2, 56, 8, 52, 14);
    snout.cubicTo(46, 18, 36, 14, 34, 4);
    painter.setBrush(QColor(45, 160, 125));
    painter.setPen(QPen(QColor(8, 40, 32), 1.5));
    painter.drawPath(snout);

    // 张口露齿
    painter.setBrush(QColor(25, 15, 12));
    QPainterPath jaw;
    jaw.moveTo(36, 8);
    jaw.cubicTo(44, 10, 48, 14, 44, 16);
    jaw.lineTo(34, 14);
    jaw.closeSubpath();
    painter.drawPath(jaw);
    painter.setPen(QColor(235, 230, 210));
    for (int i = 0; i < 4; ++i) {
        painter.drawLine(QPointF(37 + i * 2.5, 10), QPointF(37 + i * 2.5, 13));
    }

    painter.setPen(QPen(QColor(210, 235, 225), 1.4, Qt::SolidLine, Qt::RoundCap));
    const qreal whiskerWave = 4.0 * qSin(m_time * 1.8);
    for (int i = -1; i <= 1; ++i) {
        const qreal baseY = 6.0 + i * 4.0;
        QPainterPath whisker;
        whisker.moveTo(46, baseY);
        whisker.cubicTo(58, baseY - 6 + whiskerWave * i,
                        72, baseY + whiskerWave,
                        88 + whiskerWave, baseY + i * 5);
        painter.drawPath(whisker);
    }
    painter.drawLine(QPointF(22, 16), QPointF(6, 30 + whiskerWave));
    painter.drawLine(QPointF(28, 17), QPointF(18, 32 - whiskerWave));

    painter.setPen(QPen(QColor(15, 25, 20), 2.0));
    painter.setBrush(QColor(20, 35, 28));
    auto drawAntler = [&](qreal sx, qreal sy, qreal dir) {
        QPainterPath antler;
        antler.moveTo(sx, sy);
        antler.lineTo(sx + dir * 8, sy - 24);
        antler.lineTo(sx + dir * 4, sy - 12);
        antler.lineTo(sx + dir * 16, sy - 32);
        antler.lineTo(sx + dir * 6, sy - 14);
        antler.lineTo(sx + dir * 12, sy - 28);
        painter.drawPath(antler);
    };
    drawAntler(6, -6, -1);
    drawAntler(22, -8, 1);

    painter.setPen(Qt::NoPen);
    for (int i = 0; i < 7; ++i) {
        const qreal fx = 2 + i * 6;
        const qreal fw = 3.0 * qSin(m_time * 2.0 + i * 0.8);
        QPainterPath mane;
        mane.moveTo(fx, -8);
        mane.cubicTo(fx - 6 + fw, -22, fx + 4, -30 + fw, fx + 10, -18);
        mane.cubicTo(fx + 4, -12, fx, -6, fx, -8);
        QLinearGradient mg(fx, -8, fx, -28);
        mg.setColorAt(0.0, QColor(30, 120, 92));
        mg.setColorAt(1.0, QColor(55, 175, 140, 210));
        painter.setBrush(mg);
        painter.drawPath(mane);
    }

    const QPointF eyePos(30, 2);
    if (m_eyesClosed || m_mood == Mood::Sleeping) {
        painter.setPen(QPen(QColor(10, 25, 18), 2.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(24, 2), QPointF(36, 2));
    } else {
        const qreal glow = 0.65 + 0.35 * qSin(m_time * 2.5);
        painter.setBrush(QColor(5, 15, 10));
        painter.setPen(QPen(QColor(3, 10, 8), 1.5));
        painter.drawEllipse(eyePos, 9, 6);

        painter.setBrush(QColor(int(200 + 40 * glow), int(255), int(180 + 50 * glow)));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(eyePos.x() + m_pupilX, eyePos.y() + m_pupilY), 2.5, 4.5);

        painter.setOpacity(0.55 * glow);
        QRadialGradient eyeGlow(eyePos, 22);
        eyeGlow.setColorAt(0.0, QColor(200, 255, 180, 140));
        eyeGlow.setColorAt(0.5, QColor(120, 220, 100, 60));
        eyeGlow.setColorAt(1.0, QColor(80, 180, 80, 0));
        painter.setBrush(eyeGlow);
        painter.drawEllipse(eyePos, 22, 16);
        painter.setOpacity(1.0);
    }

    painter.restore();
}

void DragonWidget::drawFire(QPainter &painter, qreal intensity)
{
    const QPainterPath spine = buildSpinePath();
    const QPointF headPos = spine.pointAtPercent(0.0);
    const QPointF headTan = spine.pointAtPercent(0.035) - headPos;
    const qreal headAngle = qAtan2(headTan.y(), headTan.x()) * 180.0 / M_PI + m_headTilt;

    painter.save();
    painter.translate(headPos);
    painter.rotate(headAngle - 38.0);

    for (int i = 0; i < 8; ++i) {
        const qreal spread = (i - 3.5) * 9.0;
        const qreal puff = qSin(m_time * 3.5 + i) * 6.0;
        drawCloudBlob(painter, QPointF(52 + spread * 0.3, -4 + puff),
                      22 * intensity, 14 * intensity, 0.5 * intensity);
    }

    for (int i = 0; i < 6; ++i) {
        const qreal spread = (i - 2.5) * 8.0;
        const qreal flicker = qSin(m_time * 9.0 + i) * 4.0;
        const qreal flameH = (30.0 + flicker) * intensity;
        const qreal flameW = 10.0 * intensity;

        QPainterPath flame;
        flame.moveTo(48 + spread - flameW * 0.5, 6);
        flame.cubicTo(48 + spread - flameW, 6 - flameH * 0.35,
                      48 + spread, 6 - flameH,
                      48 + spread + flameW * 0.4, 6 - flameH * 1.15);
        flame.cubicTo(48 + spread + flameW, 6 - flameH * 0.45,
                      48 + spread + flameW * 0.5, 6,
                      48 + spread + flameW * 0.5, 6);

        QLinearGradient grad(48 + spread, 6, 48 + spread, 6 - flameH);
        grad.setColorAt(0.0, QColor(255, 250, 180, int(230 * intensity)));
        grad.setColorAt(0.35, QColor(255, 180, 50, int(210 * intensity)));
        grad.setColorAt(0.7, QColor(255, 80, 20, int(120 * intensity)));
        grad.setColorAt(1.0, QColor(200, 30, 10, int(30 * intensity)));
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
        m_lastGlobalPos = event->globalPos();
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
        emitBodyMist(10);
        showBubble(QStringLiteral("龙吟震天，云火焚空！"));
        event->accept();
    }
}

void DragonWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *feedAction = menu.addAction(QStringLiteral("献祭灵珠"));
    QAction *talkAction = menu.addAction(QStringLiteral("聆听龙谕"));
    QAction *followAction = menu.addAction(QStringLiteral("祥云引路"));
    followAction->setCheckable(true);
    followAction->setChecked(m_followMouse);
    menu.addSeparator();
    QAction *quitAction = menu.addAction(QStringLiteral("退隐云端"));

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == feedAction) {
        feedDragon();
    } else if (chosen == talkAction) {
        sayRandomLine();
    } else if (chosen == followAction) {
        m_followMouse = followAction->isChecked();
        showBubble(m_followMouse ? QStringLiteral("祥云开路，神龙随行。")
                                 : QStringLiteral("盘踞此间，威仪不动。"));
    } else if (chosen == quitAction) {
        qApp->quit();
    }
}
