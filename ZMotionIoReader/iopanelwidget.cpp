#include "iopanelwidget.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

namespace {
const QString kActiveStyle = QStringLiteral(
    "QLabel { background-color: #22c55e; border: 1px solid #15803d; border-radius: 4px; color: white; font-weight: bold; }");
const QString kInactiveStyle = QStringLiteral(
    "QLabel { background-color: #e2e8f0; border: 1px solid #cbd5e1; border-radius: 4px; color: #64748b; }");
}

IoPanelWidget::IoPanelWidget(const QString &title, int channelCount, QWidget *parent)
    : QWidget(parent)
    , m_channelCount(channelCount)
{
    setupUi(title);
}

void IoPanelWidget::setupUi(const QString &title)
{
    QGroupBox *group = new QGroupBox(title, this);
    QGridLayout *grid = new QGridLayout(group);
    grid->setSpacing(6);

    const int columns = 8;
    for (int i = 0; i < m_channelCount; ++i) {
        QLabel *label = new QLabel(QString::number(i), group);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedSize(42, 28);
        label->setStyleSheet(kInactiveStyle);
        grid->addWidget(label, i / columns, i % columns);
        m_indicators.append(label);
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(group);
}

void IoPanelWidget::setChannelState(int channel, bool active)
{
    if (channel < 0 || channel >= m_indicators.size()) {
        return;
    }
    m_indicators[channel]->setStyleSheet(active ? kActiveStyle : kInactiveStyle);
}

void IoPanelWidget::setAllStates(uint32_t bitmask)
{
    for (int i = 0; i < m_indicators.size(); ++i) {
        const bool active = (bitmask >> i) & 1U;
        m_indicators[i]->setStyleSheet(active ? kActiveStyle : kInactiveStyle);
    }
}
