#ifndef DRAGONWIDGET_H
#define DRAGONWIDGET_H

#include <QWidget>
#include <QPoint>
#include <QTimer>

class DragonWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DragonWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onAnimTick();
    void onBlink();

private:
    void drawDragon(QPainter &painter);
    void drawFire(QPainter &painter, qreal intensity);
    void scheduleNextBlink();

    QTimer m_animTimer;
    QTimer m_blinkTimer;
    QPoint m_dragOffset;
    bool m_dragging = false;
    bool m_eyesClosed = false;
    bool m_breathingFire = false;

    qreal m_time = 0.0;
    qreal m_tailAngle = 0.0;
    qreal m_wingAngle = 0.0;
    qreal m_breathScale = 1.0;
    qreal m_fireIntensity = 0.0;
    qreal m_fireStartTime = -1.0;
    qreal m_bounceY = 0.0;
};

#endif // DRAGONWIDGET_H
