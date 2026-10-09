#include "protocolselector.h"

#include <QSpinBox>

#include <util/net/netutil.h>

namespace {

inline constexpr int anyProtocol = -1;
inline constexpr int protocolMax = 255;

// The first value is for the "Custom" item: Any, TCP, UDP, ICMP, ICMPv6, IGMP, GRE, ESP, AH, SCTP
inline constexpr std::array protocolValues = { 0, anyProtocol, 6, 17, 1, 58, 2, 47, 50, 51, 132 };

}

ProtocolSelector::ProtocolSelector(bool hasAny, QWidget *parent) : SpinCombo(parent)
{
    setupUi(hasAny);
}

int ProtocolSelector::protocol() const
{
    return spinBox()->value();
}

void ProtocolSelector::setProtocol(int v)
{
    spinBox()->setValue(v);
}

bool ProtocolSelector::isAny() const
{
    return protocol() == anyProtocol;
}

void ProtocolSelector::setAny()
{
    setProtocol(anyProtocol);
}

void ProtocolSelector::retranslateUi()
{
    QStringList list = { tr("Custom") };

    const auto values = this->values().mid(1);

    for (const int value : values) {
        list << ((value == anyProtocol) ? tr("Any") : NetUtil::protocolName(value));
    }

    setNames(list);
}

void ProtocolSelector::setupUi(bool hasAny)
{
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto values = makeValuesList(protocolValues);

    if (hasAny) {
        spinBox()->setSpecialValueText(" "); // empty: a typed number replaces it
    } else {
        values.removeOne(anyProtocol);
    }

    setValues(values);

    spinBox()->setRange(hasAny ? anyProtocol : 0, protocolMax);
    spinBox()->setValue(spinBox()->minimum()); // Any or 0

    connect(spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ProtocolSelector::protocolChanged);
}
