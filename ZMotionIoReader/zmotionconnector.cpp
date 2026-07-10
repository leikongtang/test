#include "zmotionconnector.h"

namespace {
bool isCancelled(const ZMotionConnectRequest &request)
{
    return request.cancelled && request.cancelled->loadAcquire() != 0;
}

bool isDualCancelled(const ZMotionDualConnectRequest &request)
{
    return request.cancelled && request.cancelled->loadAcquire() != 0;
}

ZMotionConnectRequest makeEthRequest(const ZMotionDualConnectRequest &dual, const QString &address)
{
    ZMotionConnectRequest request;
    request.type = ConnectType::Ethernet;
    request.address = address;
    request.cancelled = dual.cancelled;
    request.token = dual.token;
    return request;
}

ZMotionConnectRequest makeSerialRequest(const ZMotionDualConnectRequest &dual, const QString &address)
{
    ZMotionConnectRequest request;
    request.type = ConnectType::Serial;
    request.address = address;
    request.baudRate = dual.baudRate;
    request.dataBits = dual.dataBits;
    request.parity = dual.parity;
    request.stopBits = dual.stopBits;
    request.cancelled = dual.cancelled;
    request.token = dual.token;
    return request;
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

QString connectModeName(ConnectMode mode)
{
    switch (mode) {
    case ConnectMode::EthInSerialOut:
        return QStringLiteral("网口输入 + 串口输出");
    case ConnectMode::Ethernet:
        return QStringLiteral("以太网");
    case ConnectMode::Serial:
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

ZMotionDualConnectResult zmotionConnectDual(const ZMotionDualConnectRequest &request)
{
    ZMotionDualConnectResult result;
    result.token = request.token;

    if (isDualCancelled(request)) {
        result.errorCode = kZMotionConnectCancelled;
        return result;
    }

    switch (request.mode) {
    case ConnectMode::EthInSerialOut: {
        const ZMotionConnectResult inputResult =
            zmotionConnect(makeEthRequest(request, request.inputAddress));
        if (inputResult.errorCode != ERR_OK) {
            result.errorCode = inputResult.errorCode;
            result.failedTarget = request.inputAddress;
            return result;
        }

        if (isDualCancelled(request)) {
            ZAux_Close(inputResult.handle);
            result.errorCode = kZMotionConnectCancelled;
            return result;
        }

        const ZMotionConnectResult outputResult =
            zmotionConnect(makeSerialRequest(request, request.outputAddress));
        if (outputResult.errorCode != ERR_OK) {
            ZAux_Close(inputResult.handle);
            result.errorCode = outputResult.errorCode;
            result.failedTarget = request.outputAddress;
            return result;
        }

        result.inputHandle = inputResult.handle;
        result.outputHandle = outputResult.handle;
        result.errorCode = ERR_OK;
        break;
    }
    case ConnectMode::Ethernet: {
        const ZMotionConnectResult ethResult =
            zmotionConnect(makeEthRequest(request, request.inputAddress));
        result.errorCode = ethResult.errorCode;
        result.failedTarget = request.inputAddress;
        result.inputHandle = ethResult.handle;
        result.outputHandle = ethResult.handle;
        break;
    }
    case ConnectMode::Serial: {
        const ZMotionConnectResult serialResult =
            zmotionConnect(makeSerialRequest(request, request.outputAddress));
        result.errorCode = serialResult.errorCode;
        result.failedTarget = request.outputAddress;
        result.inputHandle = serialResult.handle;
        result.outputHandle = serialResult.handle;
        break;
    }
    }

    return result;
}
