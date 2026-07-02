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
    void onPointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance);
    void onPrecisionChanged(double value);

private:
    void updateDistanceDisplay();

    ImageLabel *m_imageLabel;
    QPushButton *m_selectImageButton;
    QPushButton *m_clearPointsButton;
    QLabel *m_fileNameLabel;
    QLabel *m_point1Label;
    QLabel *m_point2Label;
    QLabel *m_pixelDistanceLabel;
    QLabel *m_realDistanceLabel;
    QDoubleSpinBox *m_precisionSpinBox;

    double m_pixelDistance;
};

#endif // MAINWINDOW_H
