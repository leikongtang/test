#ifndef ZMOTIONCONNECTOR_H
#define ZMOTIONCONNECTOR_H

#include "zmcaux.h"

#include <QAtomicInt>
#include <QString>

constexpr int kZMotionConnectCancelled = -100;

enum class ConnectType
{
    Ethernet = 0,
    Serial
};

enum class ConnectMode
{
    EthInSerialOut = 0,
    Ethernet,
    Serial
};

struct ZMotionConnectRequest
{
    ConnectType type = ConnectType::Ethernet;
    QString address;
    int baudRate = 115200;
    int dataBits = 8;
    int parity = 0;
    int stopBits = 1;
    QAtomicInt *cancelled = nullptr;
    quint64 token = 0;
};

struct ZMotionConnectResult
{
    int errorCode = -1;
    ZMC_HANDLE handle = nullptr;
    quint64 token = 0;
};

struct ZMotionDualConnectRequest
{
    ConnectMode mode = ConnectMode::EthInSerialOut;
    QString inputAddress;
    QString outputAddress;
    int baudRate = 115200;
    int dataBits = 8;
    int parity = 0;
    int stopBits = 1;
    QAtomicInt *cancelled = nullptr;
    quint64 token = 0;
};

struct ZMotionDualConnectResult
{
    int errorCode = -1;
    ZMC_HANDLE inputHandle = nullptr;
    ZMC_HANDLE outputHandle = nullptr;
    QString failedTarget;
    quint64 token = 0;
};

QString connectTypeName(ConnectType type);
QString connectModeName(ConnectMode mode);
ZMotionConnectResult zmotionConnect(const ZMotionConnectRequest &request);
ZMotionDualConnectResult zmotionConnectDual(const ZMotionDualConnectRequest &request);

#endif // ZMOTIONCONNECTOR_H
