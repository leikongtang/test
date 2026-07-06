#ifndef DRAGONWIDGET_H
#define DRAGONWIDGET_H

#include <QPoint>
#include <QPointF>
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
    void onBlink();
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
        Happy,
        Sleeping,
        Eating
    };

    struct Heart {
        QPointF pos;
        qreal life = 1.0;
        qreal vy = -40.0;
    };

    struct Food {
        QPointF pos;
        qreal vy = 0.0;
        bool eaten = false;
    };

    QPointF toDragonLocal(const QPoint &widgetPos) const;
    HitRegion hitTest(const QPointF &local) const;
    void triggerInteraction(HitRegion region);
    void showBubble(const QString &text, int durationMs = 2500);
    void setMood(Mood mood);
    void wakeUp();
    void feedDragon();
    void sayRandomLine();
    void spawnHearts(int count);
    void updateHearts(qreal dt);
    void updateFood(qreal dt);
    void updateFollowMouse(qreal dt);
    void updateMood(qreal dt);
    void drawDragon(QPainter &painter);
    void drawFire(QPainter &painter, qreal intensity);
    void drawBubble(QPainter &painter);
    void drawHearts(QPainter &painter);
    void drawFood(QPainter &painter);
    void drawSleepEffect(QPainter &painter);
    void scheduleNextBlink();

    QTimer m_animTimer;
    QTimer m_blinkTimer;
    QTimer m_idleTimer;

    QPoint m_dragOffset;
    QPoint m_pressPos;
    QPointF m_localMouse;
    bool m_dragging = false;
    bool m_mouseOver = false;
    bool m_followMouse = false;
    bool m_eyesClosed = false;
    bool m_breathingFire = false;

    Mood m_mood = Mood::Normal;
    qreal m_time = 0.0;
    qreal m_tailAngle = 0.0;
    qreal m_wingAngle = 0.0;
    qreal m_breathScale = 1.0;
    qreal m_fireIntensity = 0.0;
    qreal m_fireStartTime = -1.0;
    qreal m_bounceY = 0.0;
    qreal m_headTilt = 0.0;
    qreal m_happyTimer = 0.0;
    qreal m_eatTimer = 0.0;
    qreal m_bubbleAlpha = 0.0;
    qreal m_bubbleTimer = 0.0;
    qreal m_idleSeconds = 0.0;
    qreal m_pupilX = 0.0;
    qreal m_pupilY = 0.0;
    qreal m_blushAlpha = 0.0;

    QString m_bubbleText;
    QVector<Heart> m_hearts;
    QVector<Food> m_foods;
};

#endif // DRAGONWIDGET_H
