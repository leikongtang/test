#ifndef DRAGONWIDGET_H
#define DRAGONWIDGET_H

#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWidget>

class DragonWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DragonWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onAnimTick();
    void onIdleCheck();

private:
    enum class HitRegion {
        None,
        Head,
        Body,
        Tail
    };

    enum class Mood {
        Normal,
        Majestic,
        Sleeping,
        Eating
    };

    struct AuraRing {
        QPointF center;
        qreal radius = 10.0;
        qreal life = 1.0;
        qreal maxRadius = 50.0;
    };

    struct Food {
        QPointF pos;
        qreal vy = 0.0;
        bool eaten = false;
    };

    struct CloudPuff {
        QPointF pos;
        qreal size = 12.0;
        qreal alpha = 0.5;
        qreal drift = 0.0;
    };

    struct MistParticle {
        QPointF pos;
        QPointF vel;
        qreal size = 8.0;
        qreal life = 1.0;
        qreal alpha = 0.6;
    };

    bool loadDragonSprite();
    QRectF dragonDrawRect() const;
    QPointF dragonCenter() const;
    QPointF toDragonLocal(const QPoint &widgetPos) const;
    HitRegion hitTest(const QPointF &local) const;
    qreal coilOffsetAt(qreal t) const;
    QPointF spritePointAt(qreal t, qreal nx = 0.5) const;
    QPointF untwistLocal(const QPointF &local) const;
    void boostCoil(HitRegion region);
    void triggerInteraction(HitRegion region);
    void showBubble(const QString &text, int durationMs = 2500);
    void setMood(Mood mood);
    void wakeUp();
    void feedDragon();
    void sayRandomLine();
    void spawnAuraPulse(const QPointF &center);
    void emitBodyMist(int count = 3);
    void emitDragMist();
    void updateAura(qreal dt);
    void updateFood(qreal dt);
    void updateClouds(qreal dt);
    void updateMist(qreal dt);
    void updateFollowMouse(qreal dt);
    void updateMood(qreal dt);
    void drawBackgroundMist(QPainter &painter);
    void drawMistParticles(QPainter &painter);
    void drawDragonSprite(QPainter &painter);
    void drawFire(QPainter &painter, qreal intensity);
    void drawAuraRings(QPainter &painter);
    void drawBubble(QPainter &painter);
    void drawFood(QPainter &painter);
    void drawSleepEffect(QPainter &painter);

    QTimer m_animTimer;
    QTimer m_idleTimer;

    static constexpr int kCoilStrips = 48;

    QPixmap m_dragonPixmap;
    QRectF m_spriteRect;

    QPoint m_dragOffset;
    QPoint m_pressPos;
    QPoint m_lastGlobalPos;
    QPointF m_localMouse;
    bool m_dragging = false;
    bool m_mouseOver = false;
    bool m_followMouse = false;
    bool m_breathingFire = false;

    Mood m_mood = Mood::Normal;
    qreal m_time = 0.0;
    qreal m_breathScale = 1.0;
    qreal m_swayAngle = 0.0;
    qreal m_coilBoost = 0.0;
    qreal m_fireIntensity = 0.0;
    qreal m_fireStartTime = -1.0;
    qreal m_bounceY = 0.0;
    qreal m_majesticTimer = 0.0;
    qreal m_eatTimer = 0.0;
    qreal m_bubbleAlpha = 0.0;
    qreal m_bubbleTimer = 0.0;
    qreal m_idleSeconds = 0.0;
    qreal m_auraAlpha = 0.0;
    qreal m_mistEmitTimer = 0.0;
    qreal m_sleepAlpha = 1.0;

    QString m_bubbleText;
    QVector<AuraRing> m_auras;
    QVector<Food> m_foods;
    QVector<CloudPuff> m_clouds;
    QVector<MistParticle> m_mists;
};

#endif // DRAGONWIDGET_H
