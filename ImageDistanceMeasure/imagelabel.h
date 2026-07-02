#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QPoint>
#include <QRectF>
#include <QVector>

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ImageLabel(QWidget *parent = nullptr);

    void setImage(const QPixmap &pixmap);
    void clearPoints();
    QVector<QPoint> imagePoints() const;
    void zoomIn();
    void zoomOut();
    void zoomReset();
    double zoomFactor() const;
    void setPanMode(bool enabled);
    bool panMode() const;
    void setMarkersVisible(bool visible);
    bool markersVisible() const;

signals:
    void pointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance);
    void zoomFactorChanged(double factor);
    void panModeChanged(bool enabled);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRectF imageRect() const;
    QPoint widgetToImage(const QPoint &widgetPoint) const;
    QPoint imageToWidget(const QPoint &imagePoint) const;
    void updateDistance();
    void setZoomFactorAt(double factor, const QPoint &anchor);
    void emitZoomFactorChanged();

    QPixmap m_originalPixmap;
    QVector<QPoint> m_imagePoints;
    double m_pixelDistance;
    double m_zoomFactor;
    QPoint m_panOffset;
    bool m_panMode;
    bool m_markersVisible;
    bool m_panning;
    QPoint m_lastPanPos;

    static constexpr double kMinZoom = 0.1;
    static constexpr double kMaxZoom = 20.0;
    static constexpr double kZoomStep = 1.15;
};

#endif // IMAGELABEL_H
