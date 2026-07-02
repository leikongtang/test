#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class OcrClientService;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSelectAndSendClicked();

private:
    void appendLog(const QString &message);
    void loadPreview(const QString &filePath);

    QLineEdit *m_serverIpEdit;
    QPushButton *m_selectAndSendButton;
    QLabel *m_selectedFileLabel;
    QLabel *m_previewLabel;
    QPlainTextEdit *m_logEdit;

    OcrClientService *m_ocrClientService;
    QString m_selectedImagePath;
};

#endif // MAINWINDOW_H
