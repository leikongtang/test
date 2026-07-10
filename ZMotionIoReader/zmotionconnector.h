#ifndef ZMOTIONCONNECTOR_H
#define ZMOTIONCONNECTOR_H

#include "zmcaux.h"

#include <QAtomicInt>
#include <QString>

constexpr int kZMotionConnectCancelled = -100;

struct ZMotionConnectRequest
{
    QString ipAddress;
    QAtomicInt *cancelled = nullptr;
    quint64 token = 0;
};

struct ZMotionConnectResult
{
    int errorCode = -1;
    ZMC_HANDLE handle = nullptr;
    quint64 token = 0;
};

ZMotionConnectResult zmotionConnectEth(const ZMotionConnectRequest &request);

#endif // ZMOTIONCONNECTOR_H
