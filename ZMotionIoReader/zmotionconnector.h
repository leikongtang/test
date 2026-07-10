#ifndef ZMOTIONCONNECTOR_H
#define ZMOTIONCONNECTOR_H

#include "zmcaux.h"

#include <QString>

struct ZMotionConnectResult
{
    int errorCode = -1;
    ZMC_HANDLE handle = nullptr;
};

ZMotionConnectResult zmotionConnectEth(const QString &ipAddress);

#endif // ZMOTIONCONNECTOR_H
