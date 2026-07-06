#include "dragonwidget.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QCursor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QLineF>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
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
    g.setColorAt(0.0, QColor(255, 255, 255, int(210 * alpha)));
    g.setColorAt(0.45, QColor(235, 242, 250, int(130 * alpha)));
    g.setColorAt(1.0, QColor(210, 225, 240, 0));
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, w, h);
    p.drawEllipse(c + QPointF(w * 0.45, h * 0.05), w * 0.65, h * 0.55);
    p.drawEllipse(c + QPointF(-w * 0.35, h * 0.12), w * 0.5, h * 0.45);
}

QImage fadeImageBottom(const QImage &source, qreal fadeStartRatio)
{
    QImage img = source.convertToFormat(QImage::Format_ARGB32);
    const int fadeStart = int(img.height() * fadeStartRatio);
    for (int y = fadeStart; y < img.height(); ++y) {
        const qreal factor = 1.0 - qreal(y - fadeStart) / qMax(1, img.height() - fadeStart);
        for (int x = 0; x < img.width(); ++x) {
            QColor c = img.pixelColor(x, y);
            c.setAlpha(int(c.alpha() * factor * factor));
            img.setPixelColor(x, y, c);
        }
    }
    return img;
}

} // namespace

DragonWidget::DragonWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(380, 360);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    loadDragonSprite();

    for (int i = 0; i < 16; ++i) {
        CloudPuff cloud;
        cloud.pos = QPointF(-150.0 + (qrand() % 300), -110.0 + (qrand() % 220));
        cloud.size = 18.0 + (qrand() % 40);
        cloud.alpha = 0.15 + (qrand() % 40) / 100.0;
        cloud.drift = 0.15 + (qrand() % 8) / 10.0;
        m_clouds.append(cloud);
    }

    connect(&m_animTimer, &QTimer::timeout, this, &DragonWidget::onAnimTick);
    m_animTimer.start(33);

    connect(&m_idleTimer, &QTimer::timeout, this, &DragonWidget::onIdleCheck);
    m_idleTimer.start(1000);

    m_lastGlobalPos = pos();
    m_auraAlpha = 0.3;
    showBubble(QStringLiteral("神龙降世，云雾随行。"), 3000);
}

bool DragonWidget::loadDragonSprite()
{
    QImage raw(QStringLiteral(":/resources/dragon.png"));
    if (raw.isNull()) {
        return false;
    }

    const QImage faded = fadeImageBottom(raw, 0.72);
    m_dragonPixmap = QPixmap::fromImage(faded);

    const qreal targetW = 340.0;
    const qreal scale = targetW / m_dragonPixmap.width();
    const qreal targetH = m_dragonPixmap.height() * scale;
    m_dragonPixmap = m_dragonPixmap.scaled(int(targetW), int(targetH),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);

    m_spriteRect = QRectF(-m_dragonPixmap.width() / 2.0,
                          -m_dragonPixmap.height() / 2.0 + 10.0,
                          m_dragonPixmap.width(),
                          m_dragonPixmap.height());
    return true;
}

QRectF DragonWidget::dragonDrawRect() const
{
    return m_spriteRect;
}

QPointF DragonWidget::dragonCenter() const
{
    return m_spriteRect.center();
}

QPointF DragonWidget::toDragonLocal(const QPoint &widgetPos) const
{
    const qreal cx = width() / 2.0;
    const qreal cy = height() / 2.0 + m_bounceY;
    return QPointF((widgetPos.x() - cx) / m_breathScale,
                   (widgetPos.y() - cy) / m_breathScale);
}

DragonWidget::HitRegion DragonWidget::hitTest(const QPointF &local) const
{
    const QRectF r = dragonDrawRect();
    if (!r.contains(local)) {
        return HitRegion::None;
    }

    const qreal nx = (local.x() - r.left()) / r.width();
    const qreal ny = (local.y() - r.top()) / r.height();

    if (nx > 0.42 && ny < 0.38) {
        return HitRegion::Head;
    }
    if (nx < 0.38 && ny > 0.62) {
        return HitRegion::Tail;
    }
    if (ny > 0.18 && ny < 0.82) {
        return HitRegion::Body;
    }
    return HitRegion::None;
}

void DragonWidget::setMood(Mood mood)
{
    m_mood = mood;
    if (mood == Mood::Majestic) {
        m_majesticTimer = 2.8;
        m_auraAlpha = 1.0;
    } else if (mood == Mood::Normal) {
        m_auraAlpha = 0.3;
        m_sleepAlpha = 1.0;
    } else if (mood == Mood::Sleeping) {
        m_sleepAlpha = 0.55;
    }
}

void DragonWidget::wakeUp()
{
    if (m_mood == Mood::Sleeping) {
        setMood(Mood::Majestic);
        showBubble(QStringLiteral("何人惊扰神龙？"), 2200);
        m_idleSeconds = 0.0;
        emitBodyMist(10);
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
    ring.radius = 12.0;
    ring.maxRadius = 70.0;
    ring.life = 1.0;
    m_auras.append(ring);
}

void DragonWidget::emitBodyMist(int count)
{
    if (m_mists.size() > 100) {
        return;
    }
    const QRectF r = dragonDrawRect();
    for (int i = 0; i < count; ++i) {
        MistParticle m;
        m.pos = QPointF(r.left() + (qrand() % int(r.width())),
                        r.top() + (qrand() % int(r.height())));
        m.vel = QPointF((qrand() % 12) - 6, -10.0 - (qrand() % 10));
        m.size = 12.0 + (qrand() % 22);
        m.life = 1.0;
        m.alpha = 0.3 + (qrand() % 35) / 100.0;
        m_mists.append(m);
    }
}

void DragonWidget::emitDragMist()
{
    if (m_mists.size() > 100) {
        return;
    }
    const QRectF r = dragonDrawRect();
    for (int i = 0; i < 4; ++i) {
        MistParticle m;
        m.pos = r.center() + QPointF((qrand() % 80) - 40, (qrand() % 40) - 20);
        m.vel = QPointF(-14.0 - (qrand() % 10), (qrand() % 8) - 4);
        m.size = 16.0 + (qrand() % 14);
        m.life = 0.95;
        m.alpha = 0.5;
        m_mists.append(m);
    }
}

void DragonWidget::triggerInteraction(HitRegion region)
{
    wakeUp();
    m_idleSeconds = 0.0;
    const QRectF r = dragonDrawRect();

    switch (region) {
    case HitRegion::Head:
        setMood(Mood::Majestic);
        spawnAuraPulse(QPointF(r.left() + r.width() * 0.65, r.top() + r.height() * 0.22));
        emitBodyMist(10);
        showBubble(QStringLiteral("触龙首者，当思其威。"));
        break;
    case HitRegion::Body:
        setMood(Mood::Majestic);
        spawnAuraPulse(r.center());
        emitBodyMist(8);
        showBubble(QStringLiteral("龙躯不动，风云自生。"));
        break;
    case HitRegion::Tail:
        setMood(Mood::Majestic);
        spawnAuraPulse(QPointF(r.left() + r.width() * 0.25, r.top() + r.height() * 0.78));
        emitBodyMist(10);
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
    const QRectF r = dragonDrawRect();
    food.pos = QPointF(r.left() + r.width() * 0.58, r.top() + r.height() * 0.12);
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
        AuraRing &ring = m_auras[i];
        ring.radius += 50.0 * dt;
        ring.life -= dt * 0.5;
        if (ring.life <= 0.0 || ring.radius > ring.maxRadius) {
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
        food.vy += 130.0 * dt;
        food.pos.setY(food.pos.y() + food.vy * dt);
        const QRectF r = dragonDrawRect();
        if (food.pos.y() >= r.top() + r.height() * 0.2) {
            food.eaten = true;
            emitBodyMist(8);
        }
    }
}

void DragonWidget::updateClouds(qreal dt)
{
    for (CloudPuff &cloud : m_clouds) {
        cloud.pos.setX(cloud.pos.x() + cloud.drift * dt * 5.0);
        cloud.pos.setY(cloud.pos.y() - cloud.drift * dt * 2.0);
        cloud.alpha = 0.14 + 0.1 * (1.0 + qSin(m_time * 1.1 + cloud.drift * 4.0));
        if (cloud.pos.x() > 170.0) {
            cloud.pos.setX(-170.0);
        }
    }
}

void DragonWidget::updateMist(qreal dt)
{
    for (int i = m_mists.size() - 1; i >= 0; --i) {
        MistParticle &m = m_mists[i];
        m.pos += m.vel * dt;
        m.vel.setX(m.vel.x() * 0.98);
        m.vel.setY(m.vel.y() - 5.0 * dt);
        m.size += 10.0 * dt;
        m.life -= dt * 0.4;
        m.alpha *= 0.993;
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
            spawnAuraPulse(dragonCenter());
        }
    }

    if (m_auraAlpha > 0.3 && m_mood != Mood::Majestic) {
        m_auraAlpha = qMax(0.3, m_auraAlpha - dt * 0.35);
    }

    if (m_bubbleTimer > 0.0) {
        m_bubbleTimer -= dt;
    } else if (m_bubbleAlpha > 0.0) {
        m_bubbleAlpha = qMax(0.0, m_bubbleAlpha - dt * 1.8);
    }
}

void DragonWidget::onAnimTick()
{
    const qreal dt = 0.033;
    m_time += dt;

    if (m_mood == Mood::Sleeping) {
        m_breathScale = 1.0 + 0.006 * qSin(m_time * 0.7);
        m_bounceY = 1.0 * qSin(m_time * 0.45);
        m_swayAngle = 0.4 * qSin(m_time * 0.6);
    } else {
        m_breathScale = 1.0 + 0.01 * qSin(m_time * 1.1);
        m_bounceY = (m_mood == Mood::Majestic ? 3.5 : 2.0) * qSin(m_time * 0.85);
        m_swayAngle = 0.8 * qSin(m_time * 0.9);
    }

    if (m_breathingFire) {
        m_fireIntensity = qMin(1.0, m_fireIntensity + 0.06);
        if (m_time - m_fireStartTime > 2.0) {
            m_breathingFire = false;
        }
    } else {
        m_fireIntensity = qMax(0.0, m_fireIntensity - 0.04);
    }

    m_mistEmitTimer += dt;
    if (m_mistEmitTimer > 0.2) {
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

void DragonWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    drawBubble(painter);

    painter.translate(width() / 2.0, height() / 2.0 + m_bounceY);
    painter.scale(m_breathScale, m_breathScale);
    painter.rotate(m_swayAngle);

    drawBackgroundMist(painter);
    drawAuraRings(painter);
    drawDragonSprite(painter);
    drawMistParticles(painter);
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
        QPointF(-100, 30), QPointF(80, -20), QPointF(-50, 90),
        QPointF(120, 50), QPointF(0, -90), QPointF(-130, -30)
    };
    for (int i = 0; i < anchors.size(); ++i) {
        const qreal pulse = 1.0 + 0.1 * qSin(m_time * 0.7 + i);
        drawCloudBlob(painter, anchors[i], 50 * pulse, 26 * pulse,
                      0.38 + 0.12 * qSin(m_time + i));
    }
    for (const CloudPuff &cloud : m_clouds) {
        drawCloudBlob(painter, cloud.pos, cloud.size * 1.3, cloud.size * 0.65, cloud.alpha);
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

void DragonWidget::drawDragonSprite(QPainter &painter)
{
    if (m_dragonPixmap.isNull()) {
        return;
    }

    painter.save();
    painter.setOpacity(m_sleepAlpha);

    if (m_auraAlpha > 0.01) {
        QRadialGradient glow(dragonCenter(), m_spriteRect.width() * 0.55);
        glow.setColorAt(0.0, QColor(100, 200, 160, int(70 * m_auraAlpha)));
        glow.setColorAt(0.7, QColor(60, 140, 110, int(25 * m_auraAlpha)));
        glow.setColorAt(1.0, QColor(40, 100, 80, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(dragonCenter(), m_spriteRect.width() * 0.5, m_spriteRect.height() * 0.45);
    }

    painter.drawPixmap(m_spriteRect.topLeft(), m_dragonPixmap);
    painter.restore();
}

void DragonWidget::drawAuraRings(QPainter &painter)
{
    painter.save();
    for (const AuraRing &r : m_auras) {
        QPen pen(QColor(180, 220, 160, int(170 * r.life)));
        pen.setWidthF(2.5);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(r.center, r.radius, r.radius * 0.6);
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
    }
}

void DragonWidget::drawSleepEffect(QPainter &painter)
{
    painter.save();
    const QRectF r = dragonDrawRect();
    for (int i = 0; i < 4; ++i) {
        const qreal x = r.left() + r.width() * 0.5 + i * 20;
        const qreal y = r.top() + 10 - i * 12 + 4.0 * qSin(m_time * 1.4 + i);
        drawCloudBlob(painter, QPointF(x, y), 16 + i * 3, 9, 0.3);
    }
    painter.restore();
}

void DragonWidget::drawFire(QPainter &painter, qreal intensity)
{
    const QRectF r = dragonDrawRect();
    const QPointF mouth(r.left() + r.width() * 0.52, r.top() + r.height() * 0.28);

    painter.save();
    painter.translate(mouth);

    for (int i = 0; i < 8; ++i) {
        const qreal spread = (i - 3.5) * 10.0;
        const qreal puff = qSin(m_time * 3.5 + i) * 6.0;
        drawCloudBlob(painter, QPointF(spread * 0.4, -8 + puff),
                      24 * intensity, 16 * intensity, 0.55 * intensity);
    }

    for (int i = 0; i < 6; ++i) {
        const qreal spread = (i - 2.5) * 9.0;
        const qreal flicker = qSin(m_time * 9.0 + i) * 5.0;
        const qreal flameH = (32.0 + flicker) * intensity;
        const qreal flameW = 11.0 * intensity;

        QPainterPath flame;
        flame.moveTo(spread - flameW * 0.5, 0);
        flame.cubicTo(spread - flameW, -flameH * 0.35,
                      spread, -flameH,
                      spread + flameW * 0.4, -flameH * 1.12);
        flame.cubicTo(spread + flameW, -flameH * 0.45,
                      spread + flameW * 0.5, 0,
                      spread + flameW * 0.5, 0);

        QLinearGradient grad(spread, 0, spread, -flameH);
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
        emitBodyMist(12);
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
