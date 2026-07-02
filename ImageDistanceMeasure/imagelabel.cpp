#include "imagelabel.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QtMath>
#include <cmath>

ImageLabel::ImageLabel(QWidget *parent)
    : QLabel(parent)
    , m_pixelDistance(0.0)
    , m_zoomFactor(1.0)
    , m_panMode(false)
    , m_point1Visible(true)
    , m_point2Visible(true)
    , m_lineVisible(true)
    , m_guideLinesEnabled(false)
    , m_guideLineAngle1(0.0)
    , m_guideLineAngle2(0.0)
    , m_wheelAdjustTarget(WheelAdjustTarget::None)
    , m_point1Color(Qt::red)
    , m_point2Color(QColor(255, 80, 80))
    , m_guideLine1Color(QColor(255, 180, 0))
    , m_guideLine2Color(QColor(255, 220, 0))
    , m_measureLineColor(QColor(0, 200, 255))
    , m_panning(false)
{
    setAlignment(Qt::AlignCenter);
    setMinimumSize(640, 480);
    setFocusPolicy(Qt::WheelFocus);
    setMouseTracking(true);
    setStyleSheet(QStringLiteral("background-color: #2b2b2b; border: 1px solid #555;"));
}

void ImageLabel::setImage(const QPixmap &pixmap)
{
    m_originalPixmap = pixmap;
    m_zoomFactor = 1.0;
    m_panOffset = QPoint();
    clearPoints();
    emitZoomFactorChanged();
    update();
}

void ImageLabel::clearPoints()
{
    m_imagePoints.clear();
    m_pixelDistance = 0.0;
    updateDistance();
    update();
}

QVector<QPoint> ImageLabel::imagePoints() const
{
    return m_imagePoints;
}

void ImageLabel::zoomIn()
{
    setZoomFactorAt(m_zoomFactor * kZoomStep, rect().center());
}

void ImageLabel::zoomOut()
{
    setZoomFactorAt(m_zoomFactor / kZoomStep, rect().center());
}

void ImageLabel::zoomReset()
{
    m_zoomFactor = 1.0;
    m_panOffset = QPoint();
    emitZoomFactorChanged();
    update();
}

double ImageLabel::zoomFactor() const
{
    return m_zoomFactor;
}

void ImageLabel::setPanMode(bool enabled)
{
    if (m_panMode == enabled) {
        return;
    }

    m_panMode = enabled;
    if (m_panMode) {
        setCursor(Qt::OpenHandCursor);
    } else if (!m_panning) {
        unsetCursor();
    }

    emit panModeChanged(m_panMode);
}

bool ImageLabel::panMode() const
{
    return m_panMode;
}

void ImageLabel::setPoint1Visible(bool visible)
{
    if (m_point1Visible == visible) {
        return;
    }

    m_point1Visible = visible;
    update();
}

void ImageLabel::setPoint2Visible(bool visible)
{
    if (m_point2Visible == visible) {
        return;
    }

    m_point2Visible = visible;
    update();
}

void ImageLabel::setLineVisible(bool visible)
{
    if (m_lineVisible == visible) {
        return;
    }

    m_lineVisible = visible;
    update();
}

bool ImageLabel::point1Visible() const
{
    return m_point1Visible;
}

bool ImageLabel::point2Visible() const
{
    return m_point2Visible;
}

bool ImageLabel::lineVisible() const
{
    return m_lineVisible;
}

void ImageLabel::setGuideLinesEnabled(bool enabled)
{
    if (m_guideLinesEnabled == enabled) {
        return;
    }

    m_guideLinesEnabled = enabled;
    update();
}

bool ImageLabel::guideLinesEnabled() const
{
    return m_guideLinesEnabled;
}

void ImageLabel::setGuideLineAngle1(double degrees)
{
    setGuideLineAngleInternal(1, degrees, false);
}

void ImageLabel::setGuideLineAngle2(double degrees)
{
    setGuideLineAngleInternal(2, degrees, false);
}

double ImageLabel::guideLineAngle1() const
{
    return m_guideLineAngle1;
}

double ImageLabel::guideLineAngle2() const
{
    return m_guideLineAngle2;
}

void ImageLabel::setWheelAdjustTarget(WheelAdjustTarget target)
{
    m_wheelAdjustTarget = target;
}

ImageLabel::WheelAdjustTarget ImageLabel::wheelAdjustTarget() const
{
    return m_wheelAdjustTarget;
}

void ImageLabel::setPoint1Color(const QColor &color)
{
    if (!color.isValid() || m_point1Color == color) {
        return;
    }

    m_point1Color = color;
    update();
}

void ImageLabel::setPoint2Color(const QColor &color)
{
    if (!color.isValid() || m_point2Color == color) {
        return;
    }

    m_point2Color = color;
    update();
}

void ImageLabel::setGuideLine1Color(const QColor &color)
{
    if (!color.isValid() || m_guideLine1Color == color) {
        return;
    }

    m_guideLine1Color = color;
    update();
}

void ImageLabel::setGuideLine2Color(const QColor &color)
{
    if (!color.isValid() || m_guideLine2Color == color) {
        return;
    }

    m_guideLine2Color = color;
    update();
}

void ImageLabel::setMeasureLineColor(const QColor &color)
{
    if (!color.isValid() || m_measureLineColor == color) {
        return;
    }

    m_measureLineColor = color;
    update();
}

double ImageLabel::normalizeAngle(double degrees)
{
    const double normalized = std::fmod(degrees, 360.0);
    return normalized < 0.0 ? normalized + 360.0 : normalized;
}

void ImageLabel::setGuideLineAngleInternal(int index, double degrees, bool emitSignal)
{
    const double angle = normalizeAngle(degrees);
    if (index == 1) {
        if (qFuzzyCompare(angle, m_guideLineAngle1)) {
            return;
        }
        m_guideLineAngle1 = angle;
        if (emitSignal) {
            emit guideLineAngle1Changed(m_guideLineAngle1);
        }
    } else {
        if (qFuzzyCompare(angle, m_guideLineAngle2)) {
            return;
        }
        m_guideLineAngle2 = angle;
        if (emitSignal) {
            emit guideLineAngle2Changed(m_guideLineAngle2);
        }
    }
    update();
}

void ImageLabel::adjustGuideLineAngleByWheel(int index, int wheelDelta)
{
    if (wheelDelta == 0) {
        return;
    }

    const double step = (wheelDelta > 0 ? kAngleWheelStep : -kAngleWheelStep)
        * static_cast<double>(qAbs(wheelDelta)) / 120.0;
    if (index == 1) {
        setGuideLineAngleInternal(1, m_guideLineAngle1 + step, true);
    } else {
        setGuideLineAngleInternal(2, m_guideLineAngle2 + step, true);
    }
}

bool ImageLabel::pointsLocked() const
{
    return m_imagePoints.size() >= 2;
}

void ImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (m_originalPixmap.isNull()) {
        QLabel::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::MiddleButton
        || event->button() == Qt::RightButton
        || (m_panMode && event->button() == Qt::LeftButton)) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() != Qt::LeftButton || m_panMode) {
        QLabel::mousePressEvent(event);
        return;
    }

    const QPoint imagePoint = widgetToImage(event->pos());
    if (imagePoint.x() < 0 || imagePoint.y() < 0) {
        return;
    }

    if (m_imagePoints.size() >= 2) {
        return;
    }

    m_imagePoints.append(imagePoint);
    updateDistance();
    update();
}

void ImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        m_panOffset += event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        update();
        return;
    }

    QLabel::mouseMoveEvent(event);
}

void ImageLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_panning && (event->button() == Qt::LeftButton
                      || event->button() == Qt::MiddleButton
                      || event->button() == Qt::RightButton)) {
        m_panning = false;
        if (m_panMode) {
            setCursor(Qt::OpenHandCursor);
        } else {
            unsetCursor();
        }
        return;
    }

    QLabel::mouseReleaseEvent(event);
}

void ImageLabel::wheelEvent(QWheelEvent *event)
{
    if (m_originalPixmap.isNull()) {
        QLabel::wheelEvent(event);
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0) {
        return;
    }

    if (m_guideLinesEnabled && pointsLocked()) {
        if (m_wheelAdjustTarget == WheelAdjustTarget::GuideLine1) {
            adjustGuideLineAngleByWheel(1, delta);
            event->accept();
            return;
        }
        if (m_wheelAdjustTarget == WheelAdjustTarget::GuideLine2) {
            adjustGuideLineAngleByWheel(2, delta);
            event->accept();
            return;
        }
    }

    const double factor = delta > 0 ? kZoomStep : (1.0 / kZoomStep);
    setZoomFactorAt(m_zoomFactor * factor, event->pos());
    event->accept();
}

void ImageLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(43, 43, 43));

    if (m_originalPixmap.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         QStringLiteral("请先选择图片，然后在图片上点击两个点\n滚轮缩放，右键/中键拖拽平移，或开启移动模式"));
        QLabel::paintEvent(event);
        return;
    }

    const QRectF drawRect = imageRect();
    painter.drawPixmap(drawRect.toRect(), m_originalPixmap, m_originalPixmap.rect());

    if (m_imagePoints.isEmpty()) {
        QLabel::paintEvent(event);
        return;
    }

    QVector<QPoint> widgetPoints;
    widgetPoints.reserve(m_imagePoints.size());
    for (const QPoint &imagePoint : m_imagePoints) {
        widgetPoints.append(imageToWidget(imagePoint));
    }

    if (widgetPoints.size() >= 2 && m_lineVisible) {
        QPen linePen(m_measureLineColor);
        linePen.setWidth(2);
        linePen.setStyle(Qt::DashLine);
        painter.setPen(linePen);
        painter.drawLine(widgetPoints.at(0), widgetPoints.at(1));
    }

    if (widgetPoints.size() >= 2 && m_guideLinesEnabled) {
        drawGuideLine(painter, m_imagePoints.at(0), m_guideLineAngle1, m_guideLine1Color);
        drawGuideLine(painter, m_imagePoints.at(1), m_guideLineAngle2, m_guideLine2Color);
    }

    auto drawPointCoord = [&painter](const QPoint &widgetPoint, const QPoint &imagePoint) {
        const QString coordText = QStringLiteral("(%1, %2) pixel").arg(imagePoint.x()).arg(imagePoint.y());
        const QPoint textTopLeft = widgetPoint + QPoint(8, -20);
        const QFontMetrics fm(painter.font());
        QRect textBg = fm.boundingRect(QRect(0, 0, 200, 30), Qt::AlignLeft | Qt::AlignVCenter, coordText);
        textBg.moveTopLeft(textTopLeft);
        textBg.adjust(-4, -2, 4, 2);
        painter.fillRect(textBg, QColor(0, 0, 0, 160));
        painter.setPen(Qt::white);
        painter.drawText(textTopLeft.x(), textTopLeft.y() + fm.ascent(), coordText);
    };

    if (widgetPoints.size() >= 1 && m_point1Visible) {
        QPen pointPen(m_point1Color);
        pointPen.setWidth(2);
        painter.setPen(pointPen);
        painter.setBrush(m_point1Color);
        painter.drawEllipse(widgetPoints.at(0), 5, 5);
        drawPointCoord(widgetPoints.at(0), m_imagePoints.at(0));
    }

    if (widgetPoints.size() >= 2 && m_point2Visible) {
        QPen pointPen(m_point2Color);
        pointPen.setWidth(2);
        painter.setPen(pointPen);
        painter.setBrush(m_point2Color);
        painter.drawEllipse(widgetPoints.at(1), 5, 5);
        drawPointCoord(widgetPoints.at(1), m_imagePoints.at(1));
    }

    QLabel::paintEvent(event);
}

QRectF ImageLabel::imageRect() const
{
    if (m_originalPixmap.isNull()) {
        return QRectF();
    }

    const QSize widgetSize = size();
    const double baseScale = qMin(
        static_cast<double>(widgetSize.width()) / m_originalPixmap.width(),
        static_cast<double>(widgetSize.height()) / m_originalPixmap.height());

    const double scale = baseScale * m_zoomFactor;
    const double displayWidth = m_originalPixmap.width() * scale;
    const double displayHeight = m_originalPixmap.height() * scale;

    const double offsetX = (widgetSize.width() - displayWidth) / 2.0 + m_panOffset.x();
    const double offsetY = (widgetSize.height() - displayHeight) / 2.0 + m_panOffset.y();

    return QRectF(offsetX, offsetY, displayWidth, displayHeight);
}

QPoint ImageLabel::widgetToImage(const QPoint &widgetPoint) const
{
    if (m_originalPixmap.isNull()) {
        return QPoint(-1, -1);
    }

    const QRectF drawRect = imageRect();
    if (!drawRect.contains(widgetPoint)) {
        return QPoint(-1, -1);
    }

    const int imageX = qRound((widgetPoint.x() - drawRect.left()) * m_originalPixmap.width() / drawRect.width());
    const int imageY = qRound((widgetPoint.y() - drawRect.top()) * m_originalPixmap.height() / drawRect.height());
    return QPoint(imageX, imageY);
}

QPoint ImageLabel::imageToWidget(const QPoint &imagePoint) const
{
    const QRectF drawRect = imageRect();

    const int widgetX = qRound(drawRect.left() + imagePoint.x() * drawRect.width() / m_originalPixmap.width());
    const int widgetY = qRound(drawRect.top() + imagePoint.y() * drawRect.height() / m_originalPixmap.height());
    return QPoint(widgetX, widgetY);
}

void ImageLabel::drawGuideLine(QPainter &painter, const QPoint &imagePoint, double angleDegrees,
                               const QColor &color) const
{
    QPen guidePen(color);
    guidePen.setWidth(1);
    guidePen.setStyle(Qt::DashDotLine);
    painter.setPen(guidePen);

    const double radians = qDegreesToRadians(angleDegrees);
    const double dirX = qSin(radians);
    const double dirY = qCos(radians);
    const double maxLength = qSqrt(
        static_cast<double>(m_originalPixmap.width() * m_originalPixmap.width()
                            + m_originalPixmap.height() * m_originalPixmap.height()));

    const QPointF startImage(
        imagePoint.x() - dirX * maxLength,
        imagePoint.y() - dirY * maxLength);
    const QPointF endImage(
        imagePoint.x() + dirX * maxLength,
        imagePoint.y() + dirY * maxLength);

    const QRectF drawRect = imageRect();
    const auto toWidget = [&drawRect, this](const QPointF &point) {
        return QPointF(
            drawRect.left() + point.x() * drawRect.width() / m_originalPixmap.width(),
            drawRect.top() + point.y() * drawRect.height() / m_originalPixmap.height());
    };

    painter.drawLine(toWidget(startImage), toWidget(endImage));
}

void ImageLabel::updateDistance()
{
    if (m_imagePoints.size() == 2) {
        const QPoint delta = m_imagePoints.at(1) - m_imagePoints.at(0);
        m_pixelDistance = qSqrt(static_cast<double>(delta.x() * delta.x() + delta.y() * delta.y()));
    } else {
        m_pixelDistance = 0.0;
    }

    emit pointsChanged(m_imagePoints, m_pixelDistance);
}

void ImageLabel::setZoomFactorAt(double factor, const QPoint &anchor)
{
    const double clampedFactor = qBound(kMinZoom, factor, kMaxZoom);
    if (qFuzzyCompare(clampedFactor, m_zoomFactor)) {
        return;
    }

    const QRectF oldRect = imageRect();
    if (oldRect.width() > 0.0 && oldRect.height() > 0.0) {
        const double imageX = (anchor.x() - oldRect.left()) * m_originalPixmap.width() / oldRect.width();
        const double imageY = (anchor.y() - oldRect.top()) * m_originalPixmap.height() / oldRect.height();

        m_zoomFactor = clampedFactor;

        const QRectF newRect = imageRect();
        const double newLeft = anchor.x() - imageX * newRect.width() / m_originalPixmap.width();
        const double newTop = anchor.y() - imageY * newRect.height() / m_originalPixmap.height();

        m_panOffset.setX(qRound(newLeft - (width() - newRect.width()) / 2.0));
        m_panOffset.setY(qRound(newTop - (height() - newRect.height()) / 2.0));
    } else {
        m_zoomFactor = clampedFactor;
    }

    emitZoomFactorChanged();
    update();
}

void ImageLabel::emitZoomFactorChanged()
{
    emit zoomFactorChanged(m_zoomFactor);
}
