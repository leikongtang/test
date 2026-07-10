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

QString connectTypeName(ConnectType type);
ZMotionConnectResult zmotionConnect(const ZMotionConnectRequest &request);

#endif // ZMOTIONCONNECTOR_H
