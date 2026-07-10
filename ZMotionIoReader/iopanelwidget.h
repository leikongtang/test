#ifndef IOPANELWIDGET_H
#define IOPANELWIDGET_H

#include <QWidget>

class QLabel;

class IoPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IoPanelWidget(const QString &title, int channelCount, QWidget *parent = nullptr);

    void setChannelState(int channel, bool active);
    void setAllStates(uint32_t bitmask);

private:
    void setupUi(const QString &title);

    QVector<QLabel *> m_indicators;
    int m_channelCount;
};

#endif // IOPANELWIDGET_H
