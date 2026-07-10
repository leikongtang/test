#include "signalconverter.h"

uint32_t SignalConverter::convert(uint32_t inputMask, int ioCount, ConversionMode mode, int offset)
{
    if (ioCount <= 0) {
        return 0;
    }

    const uint32_t channelMask = (ioCount >= 32) ? 0xFFFFFFFFU : ((1U << ioCount) - 1U);
    inputMask &= channelMask;

    switch (mode) {
    case ConversionMode::Direct:
        return inputMask;

    case ConversionMode::Invert:
        return (~inputMask) & channelMask;

    case ConversionMode::Offset: {
        uint32_t outputMask = 0;
        for (int i = 0; i < ioCount; ++i) {
            if (!((inputMask >> i) & 1U)) {
                continue;
            }
            const int outChannel = i + offset;
            if (outChannel >= 0 && outChannel < ioCount) {
                outputMask |= (1U << outChannel);
            }
        }
        return outputMask;
    }
    }

    return inputMask;
}

QString SignalConverter::modeName(ConversionMode mode)
{
    switch (mode) {
    case ConversionMode::Direct:
        return QStringLiteral("直通");
    case ConversionMode::Invert:
        return QStringLiteral("取反");
    case ConversionMode::Offset:
        return QStringLiteral("偏移映射");
    }
    return QStringLiteral("未知");
}
