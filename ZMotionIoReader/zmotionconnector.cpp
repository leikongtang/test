#include "zmotionconnector.h"

ZMotionConnectResult zmotionConnectEth(const QString &ipAddress)
{
    ZMotionConnectResult result;
    QByteArray ipBytes = ipAddress.toLocal8Bit();
    result.errorCode = ZAux_OpenEth(ipBytes.data(), &result.handle);
    if (result.errorCode != ERR_OK) {
        result.handle = nullptr;
    }
    return result;
}
