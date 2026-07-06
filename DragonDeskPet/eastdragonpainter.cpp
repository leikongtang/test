#include "eastdragonpainter.h"

#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {

struct HitZone {
    EastDragonPainter::Part part;
    QPointF center;
    qreal radius;
};

QVector<QPointF> spinePoints(const EastDragonPainter::Pose &pose)
{
    const qreal w = pose.bodyWave;
    return {
        QPointF(-108 + w * 4, -58 + pose.waitBob),
        QPointF(-72 + w * 8, -22 + pose.waitBob * 0.6),
        QPointF(-28 + w * 10, 8 + pose.waitBob * 0.4),
        QPointF(18 + w * 6, 22 + pose.waitBob * 0.3),
        QPointF(62 + w * 2, 12 + pose.waitBob * 0.2),
        QPointF(98 - w * 4, 38 + pose.tailSway * 0.15),
        QPointF(118 - w * 6, 72 + pose.tailSway * 0.35)
    };
}

QPointF offsetForPart(const EastDragonPainter::Pose &pose,
                      EastDragonPainter::Part part,
                      const QPointF &base)
{
    if (pose.reactPart != part || pose.reactAmount <= 0.01) {
        return base;
    }
    const qreal bump = pose.reactAmount;
    switch (part) {
    case EastDragonPainter::Part::Head:
        return base + QPointF(-4 * bump, -3 * bump);
    case EastDragonPainter::Part::WhiskerLeft:
    case EastDragonPainter::Part::WhiskerRight:
        return base + QPointF(-6 * bump, 2 * bump);
    case EastDragonPainter::Part::ClawFront:
    case EastDragonPainter::Part::ClawRear:
        return base + QPointF(0, -8 * bump);
    case EastDragonPainter::Part::Tail:
        return base + QPointF(6 * bump, 4 * bump);
    default:
        return base + QPointF(0, -2 * bump);
    }
}

void drawScaleSegment(QPainter &p, const QPointF &a, const QPointF &b, qreal width)
{
    QLinearGradient grad(a, b);
    grad.setColorAt(0.0, QColor(26, 107, 74));
    grad.setColorAt(0.45, QColor(45, 154, 106));
    grad.setColorAt(1.0, QColor(78, 196, 148));
    QPen pen(grad, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.drawLine(a, b);

    const QPointF mid = (a + b) * 0.5;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(240, 232, 208, 180));
    p.drawEllipse(mid, width * 0.28, width * 0.22);
}

void drawClaw(QPainter &p, const QPointF &base, qreal angleDeg, qreal lift, bool front)
{
    p.save();
    p.translate(base);
    p.rotate(angleDeg);
    p.translate(0, -lift);

    const QColor clawColor(200, 170, 90);
    const QColor nailColor(255, 240, 200);
    p.setPen(QPen(clawColor.darker(120), 1.4));
    p.setBrush(clawColor);

    for (int i = -1; i <= 1; ++i) {
        QPainterPath toe;
        toe.moveTo(i * 5.0, 0);
        toe.cubicTo(i * 5.0 - 2, -8, i * 5.0 + (front ? 4 : 2), -16, i * 5.0 + 1, -20);
        toe.cubicTo(i * 5.0, -14, i * 5.0, -6, i * 5.0, 0);
        p.drawPath(toe);
        p.setBrush(nailColor);
        p.drawEllipse(QPointF(i * 5.0 + 1, -18), 2.2, 3.0);
        p.setBrush(clawColor);
    }
    p.restore();
}

void drawHead(QPainter &p, const EastDragonPainter::Pose &pose, const QPointF &headBase)
{
    const QPointF head = offsetForPart(pose, EastDragonPainter::Part::Head, headBase);

    p.save();
    p.translate(head);
    p.rotate(pose.headTilt);

    QRadialGradient headGrad(0, 0, 38);
    headGrad.setColorAt(0.0, QColor(60, 170, 120));
    headGrad.setColorAt(0.7, QColor(30, 120, 85));
    headGrad.setColorAt(1.0, QColor(18, 80, 58));
    p.setPen(QPen(QColor(14, 65, 48), 1.5));
    p.setBrush(headGrad);
    p.drawEllipse(QPointF(0, 0), 36, 30);

    p.setBrush(QColor(240, 232, 208));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(4, 8), 22, 14);

    // 鹿角
    p.setPen(QPen(QColor(180, 140, 70), 2.2));
    p.setBrush(QColor(200, 165, 90));
    QPainterPath hornL;
    hornL.moveTo(-8, -18);
    hornL.cubicTo(-18, -38, -28, -48, -22, -58);
    hornL.cubicTo(-14, -46, -4, -32, -8, -18);
    p.drawPath(hornL);
    QPainterPath hornR;
    hornR.moveTo(10, -16);
    hornR.cubicTo(22, -34, 32, -44, 26, -54);
    hornR.cubicTo(18, -42, 12, -28, 10, -16);
    p.drawPath(hornR);

    // 须
    const qreal ws = pose.whiskerSway;
    auto drawWhisker = [&](bool left) {
        const EastDragonPainter::Part part = left ? EastDragonPainter::Part::WhiskerLeft
                                                  : EastDragonPainter::Part::WhiskerRight;
        const QPointF start = offsetForPart(pose, part, QPointF(left ? -18 : 16, 4));
        QPen wPen(QColor(212, 192, 144), 1.6, Qt::SolidLine, Qt::RoundCap);
        if (pose.hoverPart == part) {
            wPen.setColor(QColor(255, 230, 160));
            wPen.setWidthF(2.2);
        }
        p.setPen(wPen);
        QPainterPath whisker;
        whisker.moveTo(start);
        if (left) {
            whisker.cubicTo(-34 + ws * 4, 0, -52 + ws * 6, 10, -68 + ws * 8, 18);
        } else {
            whisker.cubicTo(30 - ws * 3, -2, 46 - ws * 5, 6, 58 - ws * 6, 14);
        }
        p.drawPath(whisker);
    };
    drawWhisker(true);
    drawWhisker(false);

    // 眼
    const qreal eyeY = -2;
    for (int side : {-1, 1}) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 220, 60));
        p.drawEllipse(QPointF(side * 12, eyeY), 9, 11);
        if (pose.eyesOpen) {
            p.setBrush(QColor(20, 60, 40));
            p.drawEllipse(QPointF(side * 12, eyeY), 3, 7);
            p.setBrush(QColor(255, 255, 220, 180));
            p.drawEllipse(QPointF(side * 12 + 2, eyeY - 3), 2, 2);
        } else {
            p.setPen(QPen(QColor(20, 60, 40), 2));
            p.drawLine(QPointF(side * 12 - 6, eyeY), QPointF(side * 12 + 6, eyeY));
        }
    }

    // 鼻口
    p.setPen(QPen(QColor(14, 65, 48), 1.2));
    p.setBrush(QColor(240, 232, 208));
    p.drawEllipse(QPointF(0, 12), 10, 7);
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(-8, 14, 16, 10), 200 * 16, 140 * 16);

    p.restore();
}

void drawTailFin(QPainter &p, const QPointF &tip, qreal sway)
{
    p.save();
    p.translate(tip);
    p.rotate(-25 + sway * 0.4);

    QLinearGradient finGrad(0, 0, 0, 30);
    finGrad.setColorAt(0.0, QColor(45, 154, 106));
    finGrad.setColorAt(1.0, QColor(120, 220, 180, 120));
    p.setPen(QPen(QColor(18, 80, 58), 1.2));
    p.setBrush(finGrad);

    QPainterPath fin;
    fin.moveTo(0, 0);
    fin.cubicTo(18, 8, 24, 22, 10, 32);
    fin.cubicTo(0, 24, -8, 12, 0, 0);
    p.drawPath(fin);
    p.restore();
}

} // namespace

QRectF EastDragonPainter::bounds()
{
    return QRectF(-145, -100, 290, 200);
}

EastDragonPainter::Part EastDragonPainter::hitTest(const QPointF &local)
{
    static const HitZone zones[] = {
        {Part::Head, QPointF(-95, -55), 40},
        {Part::WhiskerLeft, QPointF(-118, -28), 22},
        {Part::WhiskerRight, QPointF(-62, -18), 22},
        {Part::ClawFront, QPointF(-8, 38), 30},
        {Part::ClawRear, QPointF(58, 22), 26},
        {Part::Tail, QPointF(112, 68), 36},
        {Part::Body, QPointF(-15, 8), 48},
    };

    Part best = Part::None;
    qreal bestDist = 1e9;
    for (const HitZone &z : zones) {
        const qreal d = QLineF(local, z.center).length();
        if (d <= z.radius && d < bestDist) {
            bestDist = d;
            best = z.part;
        }
    }
    return best;
}

QPointF EastDragonPainter::partAnchor(Part part)
{
    switch (part) {
    case Part::Head: return QPointF(-95, -55);
    case Part::WhiskerLeft: return QPointF(-118, -28);
    case Part::WhiskerRight: return QPointF(-62, -18);
    case Part::ClawFront: return QPointF(-8, 38);
    case Part::ClawRear: return QPointF(58, 22);
    case Part::Body: return QPointF(-15, 8);
    case Part::Tail: return QPointF(112, 68);
    default: return QPointF(0, 0);
    }
}

QPointF EastDragonPainter::mouthPosition(const Pose &pose)
{
    const QVector<QPointF> spine = spinePoints(pose);
    const QPointF head = offsetForPart(pose, Part::Head, spine.first());
    return head + QPointF(-8, 10);
}

void EastDragonPainter::draw(QPainter &painter, const Pose &pose,
                             qreal auraAlpha, qreal sleepAlpha)
{
    painter.save();
    painter.setOpacity(sleepAlpha);

    if (auraAlpha > 0.01) {
        QRadialGradient glow(0, 0, 120);
        glow.setColorAt(0.0, QColor(100, 200, 160, int(70 * auraAlpha)));
        glow.setColorAt(0.7, QColor(60, 140, 110, int(25 * auraAlpha)));
        glow.setColorAt(1.0, QColor(40, 100, 80, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(QPointF(0, 0), 125, 95);
    }

    const QVector<QPointF> spine = spinePoints(pose);

    // 龙身
    const qreal widths[] = {26, 30, 32, 30, 26, 20, 14};
    for (int i = 0; i < spine.size() - 1; ++i) {
        drawScaleSegment(painter, spine[i], spine[i + 1], widths[i]);
    }

    // 背脊鬃
    painter.setPen(QPen(QColor(180, 220, 160, 160), 1.2));
    painter.setBrush(QColor(120, 200, 150, 100));
    for (int i = 1; i < spine.size() - 1; ++i) {
        const QPointF &pt = spine[i];
        const qreal h = 8 + 4 * qSin(i * 1.2 + pose.bodyWave * 3);
        QPainterPath spineFin;
        spineFin.moveTo(pt);
        spineFin.lineTo(pt + QPointF(0, -h));
        spineFin.lineTo(pt + QPointF(4, -h * 0.4));
        spineFin.closeSubpath();
        painter.drawPath(spineFin);
    }

    // 爪
    const QPointF clawFront = offsetForPart(pose, Part::ClawFront, spine[2] + QPointF(8, 22));
    const QPointF clawRear = offsetForPart(pose, Part::ClawRear, spine[4] + QPointF(6, 18));
    const qreal frontLift = pose.clawLift + (pose.reactPart == Part::ClawFront ? pose.reactAmount * 10 : 0);
    const qreal rearLift = pose.clawLift * 0.5
                         + (pose.reactPart == Part::ClawRear ? pose.reactAmount * 8 : 0);

    if (pose.hoverPart == Part::ClawFront || pose.hoverPart == Part::ClawRear) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 230, 120, 40));
        painter.drawEllipse(pose.hoverPart == Part::ClawFront ? clawFront : clawRear, 34, 34);
    }

    drawClaw(painter, clawFront, -15 + pose.bodyWave * 20, frontLift, true);
    drawClaw(painter, clawRear, 10 - pose.bodyWave * 10, rearLift, false);

    drawHead(painter, pose, spine.first());
    drawTailFin(painter, offsetForPart(pose, Part::Tail, spine.last()), pose.tailSway);

    if (pose.hoverPart == Part::Head || pose.hoverPart == Part::Body
        || pose.hoverPart == Part::Tail) {
        const QPointF c = partAnchor(pose.hoverPart);
        painter.setPen(QPen(QColor(255, 230, 120, 100), 1.5));
        painter.setBrush(QColor(255, 230, 120, 25));
        painter.drawEllipse(c, 38, 38);
    }

    painter.restore();
}
