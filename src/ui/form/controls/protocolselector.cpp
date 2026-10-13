#include "protocolselector.h"

#include <QSpinBox>

#include <util/net/netutil.h>

namespace {

inline constexpr int protocolMax = 255;

// The first value is for the "Custom" item: TCP, UDP, ICMP, ICMPv6, IGMP, GRE, ESP, AH, SCTP
inline constexpr std::array protocolValues = { 0, 6, 17, 1, 58, 2, 47, 50, 51, 132 };

}

ProtocolSelector::ProtocolSelector(QWidget *parent) : SpinCombo(parent)
{
    setupUi();
}

int ProtocolSelector::protocol() const
{
    return spinBox()->value();
}

void ProtocolSelector::setProtocol(int v)
{
    spinBox()->setValue(v);
}

void ProtocolSelector::retranslateUi()
{
    QStringList list = { tr("Custom") };

    const auto values = this->values().mid(1);

    for (const int value : values) {
        list << NetUtil::protocolName(value);
    }

    setNames(list);
}

void ProtocolSelector::setupUi()
{
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    setValues(makeValuesList(protocolValues));

    spinBox()->setRange(0, protocolMax);

    connect(spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ProtocolSelector::protocolChanged);
}
