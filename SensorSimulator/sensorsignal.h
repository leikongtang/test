#ifndef SENSORSIGNAL_H
#define SENSORSIGNAL_H

#include <QString>

enum class SensorType {
    PNP,
    NPN
};

enum class LogicLevel {
    High,
    Low
};

inline LogicLevel outputLevel(SensorType type, bool triggered)
{
    if (type == SensorType::PNP) {
        return triggered ? LogicLevel::High : LogicLevel::Low;
    }
    return triggered ? LogicLevel::Low : LogicLevel::High;
}

inline QString sensorTypeName(SensorType type)
{
    return type == SensorType::PNP
        ? QStringLiteral("PNP（源型）")
        : QStringLiteral("NPN（漏型）");
}

inline QString levelText(LogicLevel level, int supplyVoltage)
{
    if (level == LogicLevel::High) {
        return QStringLiteral("%1 V（高电平）").arg(supplyVoltage);
    }
    return QStringLiteral("0 V（低电平）");
}

#endif // SENSORSIGNAL_H
