#include "zmotionconnector.h"

namespace {
bool isCancelled(const ZMotionConnectRequest &request)
{
    return request.cancelled && request.cancelled->loadAcquire() != 0;
}
}

ZMotionConnectResult zmotionConnectEth(const ZMotionConnectRequest &request)
{
    ZMotionConnectResult result;
    result.token = request.token;

    if (isCancelled(request)) {
        result.errorCode = kZMotionConnectCancelled;
        return result;
    }

    QByteArray ipBytes = request.ipAddress.toLocal8Bit();
    result.errorCode = ZAux_OpenEth(ipBytes.data(), &result.handle);
    if (result.errorCode != ERR_OK) {
        result.handle = nullptr;
    } else if (isCancelled(request)) {
        ZAux_Close(result.handle);
        result.handle = nullptr;
        result.errorCode = kZMotionConnectCancelled;
    }

    return result;
}
