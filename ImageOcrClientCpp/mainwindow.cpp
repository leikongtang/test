#include "mainwindow.h"

#include "services/imagehelper.h"
#include "services/ocrclientservice.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ocrClientService(new OcrClientService())
{
    setWindowTitle(QStringLiteral("远程图片发送客户端"));
    setFixedSize(684, 465);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *serverIpLabel = new QLabel(QStringLiteral("服务器 IP"), this);
    m_serverIpEdit = new QLineEdit(this);
    m_serverIpEdit->setText(QStringLiteral("127.0.0.1"));
    m_serverIpEdit->setFixedWidth(160);

    m_selectAndSendButton = new QPushButton(QStringLiteral("选择图片并发送"), this);
    m_selectAndSendButton->setFixedWidth(130);
    connect(m_selectAndSendButton, &QPushButton::clicked, this, &MainWindow::onSelectAndSendClicked);

    m_selectedFileLabel = new QLabel(QStringLiteral("未选择文件"), this);

    topLayout->addWidget(serverIpLabel);
    topLayout->addWidget(m_serverIpEdit);
    topLayout->addWidget(m_selectAndSendButton);
    topLayout->addWidget(m_selectedFileLabel);
    topLayout->addStretch();

    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(658, 280);
    m_previewLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setScaledContents(true);
    m_previewLabel->setText(QStringLiteral("图片预览"));

    QLabel *logLabel = new QLabel(QStringLiteral("日志"), this);
    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setFixedHeight(100);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_previewLabel);
    mainLayout->addWidget(logLabel);
    mainLayout->addWidget(m_logEdit);

    setCentralWidget(centralWidget);
}

MainWindow::~MainWindow()
{
    delete m_ocrClientService;
}

void MainWindow::onSelectAndSendClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择图片"),
        QString(),
        QStringLiteral("图片文件 (*.bmp *.jpg *.jpeg *.png *.gif *.tif *.tiff);;所有文件 (*.*)"));

    if (filePath.isEmpty()) {
        return;
    }

    m_selectedImagePath = filePath;
    m_selectedFileLabel->setText(QFileInfo(filePath).fileName());
    loadPreview(filePath);

    m_selectAndSendButton->setEnabled(false);
    appendLog(QStringLiteral("开始发送图片..."));

    QString errorMessage;
    const QString base64 = ImageHelper::fileToBase64(m_selectedImagePath, &errorMessage);
    if (base64.isEmpty()) {
        appendLog(QStringLiteral("编码失败: %1").arg(errorMessage));
        QMessageBox::critical(this, QStringLiteral("错误"), errorMessage);
        m_selectAndSendButton->setEnabled(true);
        return;
    }

    appendLog(QStringLiteral("图片已编码（原文件），Base64 长度: %1").arg(base64.length()));

    const bool ok = m_ocrClientService->sendImage(m_serverIpEdit->text(), base64, &errorMessage);
    if (ok) {
        appendLog(QStringLiteral("图片发送成功。"));
    } else {
        appendLog(QStringLiteral("发送失败: %1").arg(errorMessage));
        QMessageBox::critical(this, QStringLiteral("错误"), errorMessage);
    }

    m_selectAndSendButton->setEnabled(true);
}

void MainWindow::loadPreview(const QString &filePath)
{
    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        m_previewLabel->setText(QStringLiteral("无法预览该图片"));
        return;
    }

    m_previewLabel->setPixmap(pixmap);
}

void MainWindow::appendLog(const QString &message)
{
    const QString line = QStringLiteral("[%1] %2")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")))
                             .arg(message);
    m_logEdit->appendPlainText(line);
}
