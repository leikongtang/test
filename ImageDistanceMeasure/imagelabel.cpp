#include "imagelabel.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>

ImageLabel::ImageLabel(QWidget *parent)
    : QLabel(parent)
    , m_pixelDistance(0.0)
    , m_zoomFactor(1.0)
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

void ImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (m_originalPixmap.isNull()) {
        QLabel::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QLabel::mousePressEvent(event);
        return;
    }

    const QPoint imagePoint = widgetToImage(event->pos());
    if (imagePoint.x() < 0 || imagePoint.y() < 0) {
        return;
    }

    if (m_imagePoints.size() >= 2) {
        m_imagePoints.clear();
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
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        unsetCursor();
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
                         QStringLiteral("请先选择图片，然后在图片上点击两个点\n滚轮缩放，中键拖拽平移"));
        QLabel::paintEvent(event);
        return;
    }

    const QRectF drawRect = imageRect();
    painter.drawPixmap(drawRect.toRect(), m_originalPixmap, m_originalPixmap.rect());

    if (m_imagePoints.isEmpty()) {
        QLabel::paintEvent(event);
        return;
    }

    QPen pointPen(Qt::red);
    pointPen.setWidth(2);
    painter.setPen(pointPen);
    painter.setBrush(Qt::red);

    QVector<QPoint> widgetPoints;
    widgetPoints.reserve(m_imagePoints.size());
    for (const QPoint &imagePoint : m_imagePoints) {
        const QPoint widgetPoint = imageToWidget(imagePoint);
        widgetPoints.append(widgetPoint);
        painter.drawEllipse(widgetPoint, 5, 5);
    }

    if (widgetPoints.size() == 2) {
        QPen linePen(QColor(0, 200, 255));
        linePen.setWidth(2);
        linePen.setStyle(Qt::DashLine);
        painter.setPen(linePen);
        painter.drawLine(widgetPoints.at(0), widgetPoints.at(1));
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
