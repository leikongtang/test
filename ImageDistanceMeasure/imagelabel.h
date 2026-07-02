#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QColor>
#include <QPoint>
#include <QRectF>
#include <QVector>

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    enum class WheelAdjustTarget {
        None,
        GuideLine1,
        GuideLine2
    };

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
    void setPoint1Visible(bool visible);
    void setPoint2Visible(bool visible);
    void setPointsVisible(bool visible);
    void setLineVisible(bool visible);
    bool point1Visible() const;
    bool point2Visible() const;
    bool lineVisible() const;
    void setGuideLinesEnabled(bool enabled);
    bool guideLinesEnabled() const;
    void setGuideLineAngle1(double degrees);
    void setGuideLineAngle2(double degrees);
    double guideLineAngle1() const;
    double guideLineAngle2() const;
    void setWheelAdjustTarget(WheelAdjustTarget target);
    WheelAdjustTarget wheelAdjustTarget() const;
    void setPoint1Color(const QColor &color);
    void setPoint2Color(const QColor &color);
    void setGuideLine1Color(const QColor &color);
    void setGuideLine2Color(const QColor &color);
    void setMeasureLineColor(const QColor &color);
    bool pointsLocked() const;
    double pixelDistance() const;
    QPixmap renderResultImage(const QString &pixelDistanceText,
                              const QString &realDistanceText) const;

signals:
    void pointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance);
    void zoomFactorChanged(double factor);
    void panModeChanged(bool enabled);
    void guideLineAngle1Changed(double degrees);
    void guideLineAngle2Changed(double degrees);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    static double normalizeAngle(double degrees);
    void setGuideLineAngleInternal(int index, double degrees, bool emitSignal);
    void adjustGuideLineAngleByWheel(int index, int wheelDelta);
    QRectF imageRect() const;
    QPoint widgetToImage(const QPoint &widgetPoint) const;
    QPoint imageToWidget(const QPoint &imagePoint) const;
    void drawGuideLine(QPainter &painter, const QPoint &imagePoint, double angleDegrees,
                       const QColor &color) const;
    void drawPointCoordOnImage(QPainter &painter, const QPoint &imagePoint, int fontPixelSize) const;
    int imageDrawScale(const QPixmap &pixmap) const;
    void updateDistance();
    void setZoomFactorAt(double factor, const QPoint &anchor);
    void emitZoomFactorChanged();

    QPixmap m_originalPixmap;
    QVector<QPoint> m_imagePoints;
    double m_pixelDistance;
    double m_zoomFactor;
    QPoint m_panOffset;
    bool m_panMode;
    bool m_point1Visible;
    bool m_point2Visible;
    bool m_lineVisible;
    bool m_guideLinesEnabled;
    double m_guideLineAngle1;
    double m_guideLineAngle2;
    WheelAdjustTarget m_wheelAdjustTarget;
    QColor m_point1Color;
    QColor m_point2Color;
    QColor m_guideLine1Color;
    QColor m_guideLine2Color;
    QColor m_measureLineColor;
    bool m_panning;
    QPoint m_lastPanPos;

    static constexpr double kMinZoom = 0.1;
    static constexpr double kMaxZoom = 20.0;
    static constexpr double kZoomStep = 1.15;
    static constexpr double kAngleWheelStep = 1.0;
};

#endif // IMAGELABEL_H
