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
    QLabel *precisionLabel = new QLabel(QStringLiteral("像素精度 (每像素实际长度):"), this);
    m_precisionSpinBox = new QDoubleSpinBox(this);
    m_precisionSpinBox->setDecimals(6);
    m_precisionSpinBox->setRange(0.000001, 1000000.0);
    m_precisionSpinBox->setValue(1.0);
    m_precisionSpinBox->setSuffix(QStringLiteral(" mm/px"));
    m_precisionSpinBox->setToolTip(QStringLiteral("输入每个像素对应的实际长度，用于计算两点之间的实际距离"));
    connect(m_precisionSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onPrecisionChanged);

    m_realDistanceLabel = new QLabel(QStringLiteral("实际距离: --"), this);
    m_realDistanceLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));

    precisionLayout->addWidget(precisionLabel);
    precisionLayout->addWidget(m_precisionSpinBox);
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

void MainWindow::onPrecisionChanged(double /*value*/)
{
    updateDistanceDisplay();
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

    const double realDistance = m_pixelDistance * m_precisionSpinBox->value();
    m_realDistanceLabel->setText(
        QStringLiteral("实际距离: %1 mm").arg(realDistance, 0, 'f', 4));
}
