#ifndef SERIALPORTENUMERATOR_H
#define SERIALPORTENUMERATOR_H

#include <QString>
#include <QVector>

struct SerialPortEntry
{
    QString portName;
    QString description;
    bool isVirtual;
};

QVector<SerialPortEntry> listAvailableSerialPorts();

#endif // SERIALPORTENUMERATOR_H
