#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>

class ImageLabel;
class QCheckBox;
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
    void onExportResultClicked();
    void onZoomInClicked();
    void onZoomOutClicked();
    void onZoomResetClicked();
    void onPanModeToggled(bool checked);
    void onHidePointsToggled(bool checked);
    void onHideLineToggled(bool checked);
    void onGuideLinesToggled(bool checked);
    void onGuideLineAngle1Changed(double value);
    void onGuideLineAngle2Changed(double value);
    void onWheelAdjustGuide1Toggled(bool checked);
    void onWheelAdjustGuide2Toggled(bool checked);
    void onPoint1ColorClicked();
    void onPoint2ColorClicked();
    void onGuideLine1ColorClicked();
    void onGuideLine2ColorClicked();
    void onMeasureLineColorClicked();
    void onGuideLineAngle1Updated(double value);
    void onGuideLineAngle2Updated(double value);
    void onZoomFactorChanged(double factor);
    void onPointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance);
    void onCalibrationChanged(double value);

private:
    double pixelPrecision() const;
    void updateDistanceDisplay();
    void updatePrecisionDisplay();
    void updateHideButtonText(QPushButton *button, bool hidden, const QString &name);
    void updateGuideLineControls(const QVector<QPoint> &imagePoints);
    void updateColorButton(QPushButton *button, const QColor &color);
    QPushButton *createColorButton(const QColor &initialColor);

    ImageLabel *m_imageLabel;
    QPushButton *m_selectImageButton;
    QPushButton *m_clearPointsButton;
    QPushButton *m_exportResultButton;
    QPushButton *m_zoomInButton;
    QPushButton *m_zoomOutButton;
    QPushButton *m_zoomResetButton;
    QPushButton *m_panModeButton;
    QPushButton *m_hidePointsButton;
    QPushButton *m_hideLineButton;
    QCheckBox *m_guideLinesCheckBox;
    QDoubleSpinBox *m_guideLineAngle1SpinBox;
    QDoubleSpinBox *m_guideLineAngle2SpinBox;
    QPushButton *m_wheelAdjustGuide1Button;
    QPushButton *m_wheelAdjustGuide2Button;
    QPushButton *m_point1ColorButton;
    QPushButton *m_point2ColorButton;
    QPushButton *m_guideLine1ColorButton;
    QPushButton *m_guideLine2ColorButton;
    QPushButton *m_measureLineColorButton;
    QLabel *m_pointsLockLabel;
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
