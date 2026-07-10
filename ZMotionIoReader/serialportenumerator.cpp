#include "serialportenumerator.h"

#include <QSerialPortInfo>

namespace {
bool isVirtualSerialPort(const QSerialPortInfo &info)
{
    const QString text = (info.description() + QLatin1Char(' ') + info.manufacturer()).toLower();
    return text.contains(QStringLiteral("com0com"))
        || text.contains(QStringLiteral("virtual"))
        || text.contains(QStringLiteral("vsbc"))
        || text.contains(QStringLiteral("eterlogic"))
        || text.contains(QStringLiteral("socat"));
}
}

QVector<SerialPortEntry> listAvailableSerialPorts()
{
    QVector<SerialPortEntry> ports;
    const auto infos = QSerialPortInfo::availablePorts();
    ports.reserve(infos.size());

    for (const QSerialPortInfo &info : infos) {
        SerialPortEntry entry;
        entry.portName = info.portName();
        entry.description = info.description();
        entry.isVirtual = isVirtualSerialPort(info);
        ports.append(entry);
    }

    return ports;
}
