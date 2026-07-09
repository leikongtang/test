#ifndef SENSORCIRCUITWIDGET_H
#define SENSORCIRCUITWIDGET_H

#include "sensorsignal.h"

#include <QWidget>

class SensorCircuitWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SensorCircuitWidget(QWidget *parent = nullptr);

    void setSensorType(SensorType type);
    void setTriggered(bool triggered);
    void setSupplyVoltage(int voltage);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SensorType m_type;
    bool m_triggered;
    int m_supplyVoltage;
};

#endif // SENSORCIRCUITWIDGET_H
