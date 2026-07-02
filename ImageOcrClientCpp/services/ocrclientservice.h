#ifndef OCRCLIENTSERVICE_H
#define OCRCLIENTSERVICE_H

#include <QString>

class OcrClientService
{
public:
    static const int SendPort = 55513;
    static const int TimeoutMs = 30000;

    bool sendImage(const QString &serverIp, const QString &base64Image, QString *errorMessage = nullptr);
};

#endif // OCRCLIENTSERVICE_H
