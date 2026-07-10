#ifndef SIGNALCONVERTER_H
#define SIGNALCONVERTER_H

#include <cstdint>

#include <QString>

enum class ConversionMode
{
    Direct = 0,
    Invert,
    Offset
};

class SignalConverter
{
public:
    static uint32_t convert(uint32_t inputMask, int ioCount, ConversionMode mode, int offset);
    static QString modeName(ConversionMode mode);
};

#endif // SIGNALCONVERTER_H
