#include "imagelabel.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

ImageLabel::ImageLabel(QWidget *parent)
    : QLabel(parent)
    , m_pixelDistance(0.0)
{
    setAlignment(Qt::AlignCenter);
    setMinimumSize(640, 480);
    setStyleSheet(QStringLiteral("background-color: #2b2b2b; border: 1px solid #555;"));
}

void ImageLabel::setImage(const QPixmap &pixmap)
{
    m_originalPixmap = pixmap;
    clearPoints();
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

void ImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (m_originalPixmap.isNull() || event->button() != Qt::LeftButton) {
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

void ImageLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(43, 43, 43));

    if (m_originalPixmap.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("请先选择图片，然后在图片上点击两个点"));
        QLabel::paintEvent(event);
        return;
    }

    const QSize targetSize = size();
    const QPixmap scaledPixmap = m_originalPixmap.scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    const int offsetX = (width() - scaledPixmap.width()) / 2;
    const int offsetY = (height() - scaledPixmap.height()) / 2;
    painter.drawPixmap(offsetX, offsetY, scaledPixmap);

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

QPoint ImageLabel::widgetToImage(const QPoint &widgetPoint) const
{
    if (m_originalPixmap.isNull()) {
        return QPoint(-1, -1);
    }

    const QSize targetSize = size();
    const QPixmap scaledPixmap = m_originalPixmap.scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    const int offsetX = (width() - scaledPixmap.width()) / 2;
    const int offsetY = (height() - scaledPixmap.height()) / 2;

    const QRect imageRect(offsetX, offsetY, scaledPixmap.width(), scaledPixmap.height());
    if (!imageRect.contains(widgetPoint)) {
        return QPoint(-1, -1);
    }

    const double scaleX = static_cast<double>(m_originalPixmap.width()) / scaledPixmap.width();
    const double scaleY = static_cast<double>(m_originalPixmap.height()) / scaledPixmap.height();

    const int imageX = qRound((widgetPoint.x() - offsetX) * scaleX);
    const int imageY = qRound((widgetPoint.y() - offsetY) * scaleY);
    return QPoint(imageX, imageY);
}

QPoint ImageLabel::imageToWidget(const QPoint &imagePoint) const
{
    const QSize targetSize = size();
    const QPixmap scaledPixmap = m_originalPixmap.scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    const int offsetX = (width() - scaledPixmap.width()) / 2;
    const int offsetY = (height() - scaledPixmap.height()) / 2;

    const double scaleX = static_cast<double>(scaledPixmap.width()) / m_originalPixmap.width();
    const double scaleY = static_cast<double>(scaledPixmap.height()) / m_originalPixmap.height();

    const int widgetX = offsetX + qRound(imagePoint.x() * scaleX);
    const int widgetY = offsetY + qRound(imagePoint.y() * scaleY);
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
