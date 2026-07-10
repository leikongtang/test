#include "zmotionconnector.h"

namespace {
bool isCancelled(const ZMotionConnectRequest &request)
{
    return request.cancelled && request.cancelled->loadAcquire() != 0;
}
}

QString connectTypeName(ConnectType type)
{
    switch (type) {
    case ConnectType::Ethernet:
        return QStringLiteral("以太网");
    case ConnectType::Serial:
        return QStringLiteral("串口");
    }
    return QStringLiteral("未知");
}

ZMotionConnectResult zmotionConnect(const ZMotionConnectRequest &request)
{
    ZMotionConnectResult result;
    result.token = request.token;

    if (isCancelled(request)) {
        result.errorCode = kZMotionConnectCancelled;
        return result;
    }

    if (request.type == ConnectType::Ethernet) {
        QByteArray ipBytes = request.address.toLocal8Bit();
        result.errorCode = ZAux_OpenEth(ipBytes.data(), &result.handle);
    } else {
        ZAux_SetComDefaultBaud(request.baudRate,
                               request.dataBits,
                               request.parity,
                               request.stopBits);
        QByteArray comBytes = request.address.toLocal8Bit();
        result.errorCode = ZAux_OpenCom(comBytes.data(), &result.handle);
    }

    if (result.errorCode != ERR_OK) {
        result.handle = nullptr;
    } else if (isCancelled(request)) {
        ZAux_Close(result.handle);
        result.handle = nullptr;
        result.errorCode = kZMotionConnectCancelled;
    }

    return result;
}
