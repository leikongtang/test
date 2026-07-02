#include "mainwindow.h"

#include "imagelabel.h"

#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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
    m_fileNameLabel = new QLabel(QStringLiteral("未选择文件"), this);

    connect(m_selectImageButton, &QPushButton::clicked, this, &MainWindow::onSelectImageClicked);
    connect(m_clearPointsButton, &QPushButton::clicked, this, &MainWindow::onClearPointsClicked);

    toolLayout->addWidget(m_selectImageButton);
    toolLayout->addWidget(m_clearPointsButton);
    toolLayout->addWidget(m_fileNameLabel, 1);

    m_imageLabel = new ImageLabel(this);
    connect(m_imageLabel, &ImageLabel::pointsChanged, this, &MainWindow::onPointsChanged);

    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_point1Label = new QLabel(QStringLiteral("点1: 未选择"), this);
    m_point2Label = new QLabel(QStringLiteral("点2: 未选择"), this);
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

void MainWindow::onPointsChanged(const QVector<QPoint> &imagePoints, double pixelDistance)
{
    m_pixelDistance = pixelDistance;

    if (imagePoints.size() >= 1) {
        const QPoint p1 = imagePoints.at(0);
        m_point1Label->setText(QStringLiteral("点1: (%1, %2)").arg(p1.x()).arg(p1.y()));
    } else {
        m_point1Label->setText(QStringLiteral("点1: 未选择"));
    }

    if (imagePoints.size() >= 2) {
        const QPoint p2 = imagePoints.at(1);
        m_point2Label->setText(QStringLiteral("点2: (%1, %2)").arg(p2.x()).arg(p2.y()));
    } else {
        m_point2Label->setText(QStringLiteral("点2: 未选择"));
    }

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
