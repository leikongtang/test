#include "mainwindow.h"

#include "imagelabel.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_pixelDistance(0.0)
{
    setWindowTitle(QStringLiteral("图片两点测距工具"));
    resize(900, 700);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *toolLayout = new QHBoxLayout();
    m_selectImageButton = new QPushButton(QStringLiteral("选择图片"), this);
    m_clearPointsButton = new QPushButton(QStringLiteral("清除选点"), this);
    m_zoomInButton = new QPushButton(QStringLiteral("放大"), this);
    m_zoomOutButton = new QPushButton(QStringLiteral("缩小"), this);
    m_zoomResetButton = new QPushButton(QStringLiteral("适应窗口"), this);
    m_panModeButton = new QPushButton(QStringLiteral("移动图片"), this);
    m_panModeButton->setCheckable(true);
    m_panModeButton->setToolTip(QStringLiteral("开启后可用左键拖拽移动图片"));
    m_hidePoint1Button = new QPushButton(QStringLiteral("隐藏点1"), this);
    m_hidePoint1Button->setCheckable(true);
    m_hidePoint1Button->setToolTip(QStringLiteral("隐藏或显示第一个选点"));
    m_hidePoint2Button = new QPushButton(QStringLiteral("隐藏点2"), this);
    m_hidePoint2Button->setCheckable(true);
    m_hidePoint2Button->setToolTip(QStringLiteral("隐藏或显示第二个选点"));
    m_hideLineButton = new QPushButton(QStringLiteral("隐藏线段"), this);
    m_hideLineButton->setCheckable(true);
    m_hideLineButton->setToolTip(QStringLiteral("隐藏或显示两点之间的连线"));
    m_fileNameLabel = new QLabel(QStringLiteral("未选择文件"), this);
    m_zoomLabel = new QLabel(QStringLiteral("缩放: 100%"), this);

    connect(m_selectImageButton, &QPushButton::clicked, this, &MainWindow::onSelectImageClicked);
    connect(m_clearPointsButton, &QPushButton::clicked, this, &MainWindow::onClearPointsClicked);
    connect(m_zoomInButton, &QPushButton::clicked, this, &MainWindow::onZoomInClicked);
    connect(m_zoomOutButton, &QPushButton::clicked, this, &MainWindow::onZoomOutClicked);
    connect(m_zoomResetButton, &QPushButton::clicked, this, &MainWindow::onZoomResetClicked);
    connect(m_panModeButton, &QPushButton::toggled, this, &MainWindow::onPanModeToggled);
    connect(m_hidePoint1Button, &QPushButton::toggled, this, &MainWindow::onHidePoint1Toggled);
    connect(m_hidePoint2Button, &QPushButton::toggled, this, &MainWindow::onHidePoint2Toggled);
    connect(m_hideLineButton, &QPushButton::toggled, this, &MainWindow::onHideLineToggled);

    toolLayout->addWidget(m_selectImageButton);
    toolLayout->addWidget(m_clearPointsButton);
    toolLayout->addWidget(m_zoomInButton);
    toolLayout->addWidget(m_zoomOutButton);
    toolLayout->addWidget(m_zoomResetButton);
    toolLayout->addWidget(m_panModeButton);
    toolLayout->addWidget(m_hidePoint1Button);
    toolLayout->addWidget(m_hidePoint2Button);
    toolLayout->addWidget(m_hideLineButton);
    toolLayout->addWidget(m_zoomLabel);
    toolLayout->addWidget(m_fileNameLabel, 1);

    m_imageLabel = new ImageLabel(this);
    connect(m_imageLabel, &ImageLabel::pointsChanged, this, &MainWindow::onPointsChanged);
    connect(m_imageLabel, &ImageLabel::zoomFactorChanged, this, &MainWindow::onZoomFactorChanged);
    connect(m_imageLabel, &ImageLabel::guideLineAngle1Changed, this, &MainWindow::onGuideLineAngle1Updated);
    connect(m_imageLabel, &ImageLabel::guideLineAngle2Changed, this, &MainWindow::onGuideLineAngle2Updated);

    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_point1Label = new QLabel(QStringLiteral("点1像素: 未选择"), this);
    m_point2Label = new QLabel(QStringLiteral("点2像素: 未选择"), this);
    m_pixelDistanceLabel = new QLabel(QStringLiteral("像素距离: --"), this);
    infoLayout->addWidget(m_point1Label);
    infoLayout->addWidget(m_point2Label);
    infoLayout->addWidget(m_pixelDistanceLabel);
    infoLayout->addStretch();

    QHBoxLayout *precisionLayout = new QHBoxLayout();
    QLabel *referenceLengthLabel = new QLabel(QStringLiteral("标定长度:"), this);
    m_referenceLengthSpinBox = new QDoubleSpinBox(this);
    m_referenceLengthSpinBox->setDecimals(4);
    m_referenceLengthSpinBox->setRange(0.0, 1000000.0);
    m_referenceLengthSpinBox->setValue(1.0);
    m_referenceLengthSpinBox->setSuffix(QStringLiteral(" mm"));
    m_referenceLengthSpinBox->setToolTip(QStringLiteral("输入已知物体的实际长度"));
    connect(m_referenceLengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onCalibrationChanged);

    QLabel *referencePixelLabel = new QLabel(QStringLiteral("标定像素:"), this);
    m_referencePixelSpinBox = new QDoubleSpinBox(this);
    m_referencePixelSpinBox->setDecimals(2);
    m_referencePixelSpinBox->setRange(0.01, 1000000.0);
    m_referencePixelSpinBox->setValue(1.0);
    m_referencePixelSpinBox->setSuffix(QStringLiteral(" pixel"));
    m_referencePixelSpinBox->setToolTip(QStringLiteral("输入该物体在图片上对应的像素长度"));
    connect(m_referencePixelSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onCalibrationChanged);

    m_precisionLabel = new QLabel(QStringLiteral("像素精度: 1.000000 mm/pixel"), this);
    m_precisionLabel->setToolTip(QStringLiteral("像素精度 = 标定长度 ÷ 标定像素"));

    m_realDistanceLabel = new QLabel(QStringLiteral("实际距离: --"), this);
    m_realDistanceLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));

    precisionLayout->addWidget(referenceLengthLabel);
    precisionLayout->addWidget(m_referenceLengthSpinBox);
    precisionLayout->addSpacing(12);
    precisionLayout->addWidget(referencePixelLabel);
    precisionLayout->addWidget(m_referencePixelSpinBox);
    precisionLayout->addSpacing(12);
    precisionLayout->addWidget(m_precisionLabel);
    precisionLayout->addSpacing(20);
    precisionLayout->addWidget(m_realDistanceLabel);
    precisionLayout->addStretch();

    mainLayout->addLayout(toolLayout);
    mainLayout->addWidget(m_imageLabel, 1);

    QHBoxLayout *guideLayout = new QHBoxLayout();
    m_guideLinesCheckBox = new QCheckBox(QStringLiteral("垂直辅助线"), this);
    m_guideLinesCheckBox->setToolTip(QStringLiteral("选中两点后可显示过两点的辅助线"));
    connect(m_guideLinesCheckBox, &QCheckBox::toggled, this, &MainWindow::onGuideLinesToggled);

    QLabel *guideAngle1Label = new QLabel(QStringLiteral("点1角度:"), this);
    m_guideLineAngle1SpinBox = new QDoubleSpinBox(this);
    m_guideLineAngle1SpinBox->setDecimals(1);
    m_guideLineAngle1SpinBox->setRange(0.0, 360.0);
    m_guideLineAngle1SpinBox->setWrapping(true);
    m_guideLineAngle1SpinBox->setSuffix(QStringLiteral(" °"));
    m_guideLineAngle1SpinBox->setEnabled(false);
    connect(m_guideLineAngle1SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onGuideLineAngle1Changed);

    m_wheelAdjustGuide1Button = new QPushButton(QStringLiteral("滚轮调点1"), this);
    m_wheelAdjustGuide1Button->setCheckable(true);
    m_wheelAdjustGuide1Button->setEnabled(false);
    m_wheelAdjustGuide1Button->setToolTip(QStringLiteral("开启后在图片上滚轮调整点1辅助线角度"));
    connect(m_wheelAdjustGuide1Button, &QPushButton::toggled, this, &MainWindow::onWheelAdjustGuide1Toggled);

    QLabel *guideAngle2Label = new QLabel(QStringLiteral("点2角度:"), this);
    m_guideLineAngle2SpinBox = new QDoubleSpinBox(this);
    m_guideLineAngle2SpinBox->setDecimals(1);
    m_guideLineAngle2SpinBox->setRange(0.0, 360.0);
    m_guideLineAngle2SpinBox->setWrapping(true);
    m_guideLineAngle2SpinBox->setSuffix(QStringLiteral(" °"));
    m_guideLineAngle2SpinBox->setEnabled(false);
    connect(m_guideLineAngle2SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onGuideLineAngle2Changed);

    m_wheelAdjustGuide2Button = new QPushButton(QStringLiteral("滚轮调点2"), this);
    m_wheelAdjustGuide2Button->setCheckable(true);
    m_wheelAdjustGuide2Button->setEnabled(false);
    m_wheelAdjustGuide2Button->setToolTip(QStringLiteral("开启后在图片上滚轮调整点2辅助线角度"));
    connect(m_wheelAdjustGuide2Button, &QPushButton::toggled, this, &MainWindow::onWheelAdjustGuide2Toggled);

    m_pointsLockLabel = new QLabel(QStringLiteral(""), this);
    m_pointsLockLabel->setStyleSheet(QStringLiteral("color: #ff9800;"));

    guideLayout->addWidget(m_guideLinesCheckBox);
    guideLayout->addWidget(guideAngle1Label);
    guideLayout->addWidget(m_guideLineAngle1SpinBox);
    guideLayout->addWidget(m_wheelAdjustGuide1Button);
    guideLayout->addSpacing(8);
    guideLayout->addWidget(guideAngle2Label);
    guideLayout->addWidget(m_guideLineAngle2SpinBox);
    guideLayout->addWidget(m_wheelAdjustGuide2Button);
    guideLayout->addWidget(m_pointsLockLabel);
    guideLayout->addStretch();
    mainLayout->addLayout(guideLayout);

    QHBoxLayout *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel(QStringLiteral("点1颜色:"), this));
    m_point1ColorButton = createColorButton(Qt::red);
    connect(m_point1ColorButton, &QPushButton::clicked, this, &MainWindow::onPoint1ColorClicked);
    colorLayout->addWidget(m_point1ColorButton);

    colorLayout->addWidget(new QLabel(QStringLiteral("点2颜色:"), this));
    m_point2ColorButton = createColorButton(QColor(255, 80, 80));
    connect(m_point2ColorButton, &QPushButton::clicked, this, &MainWindow::onPoint2ColorClicked);
    colorLayout->addWidget(m_point2ColorButton);

    colorLayout->addWidget(new QLabel(QStringLiteral("辅助线1:"), this));
    m_guideLine1ColorButton = createColorButton(QColor(255, 180, 0));
    connect(m_guideLine1ColorButton, &QPushButton::clicked, this, &MainWindow::onGuideLine1ColorClicked);
    colorLayout->addWidget(m_guideLine1ColorButton);

    colorLayout->addWidget(new QLabel(QStringLiteral("辅助线2:"), this));
    m_guideLine2ColorButton = createColorButton(QColor(255, 220, 0));
    connect(m_guideLine2ColorButton, &QPushButton::clicked, this, &MainWindow::onGuideLine2ColorClicked);
    colorLayout->addWidget(m_guideLine2ColorButton);

    colorLayout->addWidget(new QLabel(QStringLiteral("测距线:"), this));
    m_measureLineColorButton = createColorButton(QColor(0, 200, 255));
    connect(m_measureLineColorButton, &QPushButton::clicked, this, &MainWindow::onMeasureLineColorClicked);
    colorLayout->addWidget(m_measureLineColorButton);
    colorLayout->addStretch();
    mainLayout->addLayout(colorLayout);

    m_imageLabel->setPoint1Color(Qt::red);
    m_imageLabel->setPoint2Color(QColor(255, 80, 80));
    m_imageLabel->setGuideLine1Color(QColor(255, 180, 0));
    m_imageLabel->setGuideLine2Color(QColor(255, 220, 0));
    m_imageLabel->setMeasureLineColor(QColor(0, 200, 255));

    mainLayout->addLayout(infoLayout);
    mainLayout->addLayout(precisionLayout);

    setCentralWidget(centralWidget);
}

void MainWindow::onSelectImageClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择图片"),
        QString(),
        QStringLiteral("图片文件 (*.bmp *.jpg *.jpeg *.png *.gif *.tif *.tiff);;所有文件 (*.*)"));

    if (filePath.isEmpty()) {
        return;
    }

    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        m_fileNameLabel->setText(QStringLiteral("图片加载失败"));
        return;
    }

    m_imageLabel->setImage(pixmap);
    m_fileNameLabel->setText(QFileInfo(filePath).fileName());
    m_pixelDistance = 0.0;
    onPointsChanged(QVector<QPoint>(), 0.0);
}

void MainWindow::onClearPointsClicked()
{
    m_imageLabel->clearPoints();
}

void MainWindow::onZoomInClicked()
{
    m_imageLabel->zoomIn();
}

void MainWindow::onZoomOutClicked()
{
    m_imageLabel->zoomOut();
}

void MainWindow::onZoomResetClicked()
{
    m_imageLabel->zoomReset();
}

void MainWindow::onPanModeToggled(bool checked)
{
    m_imageLabel->setPanMode(checked);
}

void MainWindow::onHidePoint1Toggled(bool checked)
{
    m_imageLabel->setPoint1Visible(!checked);
    updateHideButtonText(m_hidePoint1Button, checked, QStringLiteral("点1"));
}

void MainWindow::onHidePoint2Toggled(bool checked)
{
    m_imageLabel->setPoint2Visible(!checked);
    updateHideButtonText(m_hidePoint2Button, checked, QStringLiteral("点2"));
}

void MainWindow::onHideLineToggled(bool checked)
{
    m_imageLabel->setLineVisible(!checked);
    updateHideButtonText(m_hideLineButton, checked, QStringLiteral("线段"));
}

void MainWindow::onGuideLinesToggled(bool checked)
{
    m_imageLabel->setGuideLinesEnabled(checked);
    updateGuideLineControls(m_imageLabel->imagePoints());
}

void MainWindow::onGuideLineAngle1Changed(double value)
{
    m_imageLabel->setGuideLineAngle1(value);
}

void MainWindow::onGuideLineAngle2Changed(double value)
{
    m_imageLabel->setGuideLineAngle2(value);
}

void MainWindow::onWheelAdjustGuide1Toggled(bool checked)
{
    if (checked) {
        m_wheelAdjustGuide2Button->setChecked(false);
        m_imageLabel->setWheelAdjustTarget(ImageLabel::WheelAdjustTarget::GuideLine1);
    } else if (!m_wheelAdjustGuide2Button->isChecked()) {
        m_imageLabel->setWheelAdjustTarget(ImageLabel::WheelAdjustTarget::None);
    }
}

void MainWindow::onWheelAdjustGuide2Toggled(bool checked)
{
    if (checked) {
        m_wheelAdjustGuide1Button->setChecked(false);
        m_imageLabel->setWheelAdjustTarget(ImageLabel::WheelAdjustTarget::GuideLine2);
    } else if (!m_wheelAdjustGuide1Button->isChecked()) {
        m_imageLabel->setWheelAdjustTarget(ImageLabel::WheelAdjustTarget::None);
    }
}

void MainWindow::onPoint1ColorClicked()
{
    const QColor currentColor(m_point1ColorButton->property("colorValue").toString());
    const QColor color = QColorDialog::getColor(currentColor,
                                                this,
                                                QStringLiteral("选择点1颜色"));
    if (!color.isValid()) {
        return;
    }

    m_imageLabel->setPoint1Color(color);
    updateColorButton(m_point1ColorButton, color);
}

void MainWindow::onPoint2ColorClicked()
{
    const QColor currentColor(m_point2ColorButton->property("colorValue").toString());
    const QColor color = QColorDialog::getColor(currentColor,
                                                this,
                                                QStringLiteral("选择点2颜色"));
    if (!color.isValid()) {
        return;
    }

    m_imageLabel->setPoint2Color(color);
    updateColorButton(m_point2ColorButton, color);
}

void MainWindow::onGuideLine1ColorClicked()
{
    const QColor currentColor(m_guideLine1ColorButton->property("colorValue").toString());
    const QColor color = QColorDialog::getColor(currentColor,
                                                this,
                                                QStringLiteral("选择辅助线1颜色"));
    if (!color.isValid()) {
        return;
    }

    m_imageLabel->setGuideLine1Color(color);
    updateColorButton(m_guideLine1ColorButton, color);
}

void MainWindow::onGuideLine2ColorClicked()
{
    const QColor currentColor(m_guideLine2ColorButton->property("colorValue").toString());
    const QColor color = QColorDialog::getColor(currentColor,
                                                this,
                                                QStringLiteral("选择辅助线2颜色"));
    if (!color.isValid()) {
        return;
    }

    m_imageLabel->setGuideLine2Color(color);
    updateColorButton(m_guideLine2ColorButton, color);
}

void MainWindow::onMeasureLineColorClicked()
{
    const QColor currentColor(m_measureLineColorButton->property("colorValue").toString());
    const QColor color = QColorDialog::getColor(currentColor,
                                                this,
                                                QStringLiteral("选择测距线颜色"));
    if (!color.isValid()) {
        return;
    }

    m_imageLabel->setMeasureLineColor(color);
    updateColorButton(m_measureLineColorButton, color);
}

void MainWindow::onGuideLineAngle1Updated(double value)
{
    const QSignalBlocker blocker(m_guideLineAngle1SpinBox);
    m_guideLineAngle1SpinBox->setValue(value);
}

void MainWindow::onGuideLineAngle2Updated(double value)
{
    const QSignalBlocker blocker(m_guideLineAngle2SpinBox);
    m_guideLineAngle2SpinBox->setValue(value);
}

void MainWindow::updateGuideLineControls(const QVector<QPoint> &imagePoints)
{
    const bool hasTwoPoints = imagePoints.size() >= 2;
    const bool guideEnabled = m_guideLinesCheckBox->isChecked() && hasTwoPoints;
    m_guideLineAngle1SpinBox->setEnabled(guideEnabled);
    m_guideLineAngle2SpinBox->setEnabled(guideEnabled);
    m_wheelAdjustGuide1Button->setEnabled(guideEnabled);
    m_wheelAdjustGuide2Button->setEnabled(guideEnabled);
    if (!guideEnabled) {
        m_wheelAdjustGuide1Button->setChecked(false);
        m_wheelAdjustGuide2Button->setChecked(false);
        m_imageLabel->setWheelAdjustTarget(ImageLabel::WheelAdjustTarget::None);
    }
    m_pointsLockLabel->setText(hasTwoPoints
        ? QStringLiteral("选点已锁定，请点击「清除选点」后重新选择")
        : QString());
}

QPushButton *MainWindow::createColorButton(const QColor &initialColor)
{
    QPushButton *button = new QPushButton(this);
    button->setFixedSize(28, 22);
    updateColorButton(button, initialColor);
    return button;
}

void MainWindow::updateColorButton(QPushButton *button, const QColor &color)
{
    button->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid #888;")
                              .arg(color.name()));
    button->setProperty("colorValue", color.name());
}

void MainWindow::updateHideButtonText(QPushButton *button, bool hidden, const QString &name)
{
    button->setText(hidden ? QStringLiteral("显示%1").arg(name) : QStringLiteral("隐藏%1").arg(name));
}

void MainWindow::onZoomFactorChanged(double factor)
{
    m_zoomLabel->setText(QStringLiteral("缩放: %1%").arg(qRound(factor * 100.0)));
}

void MainWindow::onPointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance)
{
    m_pixelDistance = pixelDistance;

    if (imagePoints.size() >= 1) {
        const QPoint p1 = imagePoints.at(0);
        m_point1Label->setText(
            QStringLiteral("点1像素: (%1, %2) pixel").arg(p1.x()).arg(p1.y()));
    } else {
        m_point1Label->setText(QStringLiteral("点1像素: 未选择"));
    }

    if (imagePoints.size() >= 2) {
        const QPoint p2 = imagePoints.at(1);
        m_point2Label->setText(
            QStringLiteral("点2像素: (%1, %2) pixel").arg(p2.x()).arg(p2.y()));
    } else {
        m_point2Label->setText(QStringLiteral("点2像素: 未选择"));
    }

    updateGuideLineControls(imagePoints);
    updateDistanceDisplay();
}

void MainWindow::onCalibrationChanged(double /*value*/)
{
    updatePrecisionDisplay();
    updateDistanceDisplay();
}

double MainWindow::pixelPrecision() const
{
    const double referencePixels = m_referencePixelSpinBox->value();
    if (referencePixels <= 0.0) {
        return 0.0;
    }

    return m_referenceLengthSpinBox->value() / referencePixels;
}

void MainWindow::updatePrecisionDisplay()
{
    const double precision = pixelPrecision();
    if (precision <= 0.0) {
        m_precisionLabel->setText(QStringLiteral("像素精度: --"));
        return;
    }

    m_precisionLabel->setText(
        QStringLiteral("像素精度: %1 mm/pixel").arg(precision, 0, 'f', 6));
}

void MainWindow::updateDistanceDisplay()
{
    if (m_pixelDistance <= 0.0) {
        m_pixelDistanceLabel->setText(QStringLiteral("像素距离: --"));
        m_realDistanceLabel->setText(QStringLiteral("实际距离: --"));
        return;
    }

    m_pixelDistanceLabel->setText(
        QStringLiteral("像素距离: %1 px").arg(m_pixelDistance, 0, 'f', 2));

    const double precision = pixelPrecision();
    if (precision <= 0.0) {
        m_realDistanceLabel->setText(QStringLiteral("实际距离: --"));
        return;
    }

    const double realDistance = m_pixelDistance * precision;
    m_realDistanceLabel->setText(
        QStringLiteral("实际距离: %1 mm").arg(realDistance, 0, 'f', 4));
}
