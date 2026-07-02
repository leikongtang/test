#include "imagehelper.h"

#include <QByteArray>
#include <QFile>

QString ImageHelper::fileToBase64(const QString &filePath, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法读取文件: %1").arg(file.errorString());
        }
        return QString();
    }

    const QByteArray bytes = file.readAll();
    return QString::fromLatin1(bytes.toBase64());
}
