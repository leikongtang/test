#ifndef EASTDRAGONPAINTER_H
#define EASTDRAGONPAINTER_H

#include <QPointF>
#include <QRectF>
#include <QVector>

class QPainter;

class EastDragonPainter
{
public:
    enum class Part {
        None,
        Head,
        WhiskerLeft,
        WhiskerRight,
        ClawFront,
        ClawRear,
        Body,
        Tail
    };

    struct Pose {
        qreal bodyWave = 0.0;
        qreal tailSway = 0.0;
        qreal headTilt = 0.0;
        qreal whiskerSway = 0.0;
        qreal clawLift = 0.0;
        qreal waitBob = 0.0;
        bool eyesOpen = true;
        Part reactPart = Part::None;
        qreal reactAmount = 0.0;
        Part hoverPart = Part::None;
    };

    static QRectF bounds();
    static Part hitTest(const QPointF &local);
    static QPointF partAnchor(Part part);
    static QPointF mouthPosition(const Pose &pose);

    static void draw(QPainter &painter, const Pose &pose,
                     qreal auraAlpha, qreal sleepAlpha);
};

#endif // EASTDRAGONPAINTER_H
