#include "ocrclientservice.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QThread>

bool OcrClientService::sendImage(const QString &serverIp, const QString &base64Image, QString *errorMessage)
{
    if (serverIp.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("服务器 IP 不能为空。");
        }
        return false;
    }

    if (base64Image.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("图片 Base64 数据不能为空。");
        }
        return false;
    }

    QJsonObject payloadObject;
    payloadObject.insert(QStringLiteral("image"), base64Image);

    QByteArray payload = QJsonDocument(payloadObject).toJson(QJsonDocument::Compact);
    payload.append('\n');

    QTcpSocket socket;
    socket.connectToHost(serverIp.trimmed(), SendPort);

    if (!socket.waitForConnected(TimeoutMs)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("连接发送端口 %1 失败: %2")
                                  .arg(SendPort)
                                  .arg(socket.errorString());
        }
        return false;
    }

    const qint64 written = socket.write(payload);
    if (written != payload.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("发送数据不完整。");
        }
        return false;
    }

    if (!socket.waitForBytesWritten(TimeoutMs)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("发送数据超时: %1").arg(socket.errorString());
        }
        return false;
    }

    QThread::msleep(200);
    socket.disconnectFromHost();
    socket.waitForDisconnected(1000);

    return true;
}
