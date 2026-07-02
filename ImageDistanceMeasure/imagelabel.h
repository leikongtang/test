#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QPoint>
#include <QVector>

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ImageLabel(QWidget *parent = nullptr);

    void setImage(const QPixmap &pixmap);
    void clearPoints();
    QVector<QPoint> imagePoints() const;

signals:
    void pointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint widgetToImage(const QPoint &widgetPoint) const;
    QPoint imageToWidget(const QPoint &imagePoint) const;
    void updateDistance();

    QPixmap m_originalPixmap;
    QVector<QPoint> m_imagePoints;
    double m_pixelDistance;
};

#endif // IMAGELABEL_H
