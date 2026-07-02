#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ImageLabel;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onSelectImageClicked();
    void onClearPointsClicked();
    void onZoomInClicked();
    void onZoomOutClicked();
    void onZoomResetClicked();
    void onZoomFactorChanged(double factor);
    void onPointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance);
    void onCalibrationChanged(double value);

private:
    double pixelPrecision() const;
    void updateDistanceDisplay();
    void updatePrecisionDisplay();

    ImageLabel *m_imageLabel;
    QPushButton *m_selectImageButton;
    QPushButton *m_clearPointsButton;
    QPushButton *m_zoomInButton;
    QPushButton *m_zoomOutButton;
    QPushButton *m_zoomResetButton;
    QLabel *m_fileNameLabel;
    QLabel *m_zoomLabel;
    QLabel *m_point1Label;
    QLabel *m_point2Label;
    QLabel *m_pixelDistanceLabel;
    QLabel *m_realDistanceLabel;
    QLabel *m_precisionLabel;
    QDoubleSpinBox *m_referenceLengthSpinBox;
    QDoubleSpinBox *m_referencePixelSpinBox;

    double m_pixelDistance;
};

#endif // MAINWINDOW_H
