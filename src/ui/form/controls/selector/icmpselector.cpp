#include "icmpselector.h"

namespace {

inline constexpr int customValue = -1;
inline constexpr qint8 typeItemCode = -1;

struct IcmpItem
{
    bool isIcmp6;
    qint8 code; // typeItemCode for the type's item
    quint8 type;
    const char *name; // also the translated text's source
};

// The types, each followed by its codes
inline constexpr IcmpItem g_icmpItems[] = {
    // ICMP
    { false, typeItemCode, 0, QT_TRANSLATE_NOOP("IcmpSelector", "Echo Reply") },
    { false, typeItemCode, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Destination Unreachable") },
    { false, 0, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Network Unreachable") },
    { false, 1, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Host Unreachable") },
    { false, 2, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Protocol Unreachable") },
    { false, 3, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Port Unreachable") },
    { false, 4, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Fragmentation Needed") },
    { false, 5, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Source Route Failed") },
    { false, 13, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Administratively Prohibited") },
    { false, typeItemCode, 5, QT_TRANSLATE_NOOP("IcmpSelector", "Redirect") },
    { false, 0, 5, QT_TRANSLATE_NOOP("IcmpSelector", "Redirect for Network") },
    { false, 1, 5, QT_TRANSLATE_NOOP("IcmpSelector", "Redirect for Host") },
    { false, typeItemCode, 8, QT_TRANSLATE_NOOP("IcmpSelector", "Echo Request") },
    { false, typeItemCode, 9, QT_TRANSLATE_NOOP("IcmpSelector", "Router Advertisement") },
    { false, typeItemCode, 10, QT_TRANSLATE_NOOP("IcmpSelector", "Router Solicitation") },
    { false, typeItemCode, 11, QT_TRANSLATE_NOOP("IcmpSelector", "Time Exceeded") },
    { false, 0, 11, QT_TRANSLATE_NOOP("IcmpSelector", "TTL Exceeded in Transit") },
    { false, 1, 11, QT_TRANSLATE_NOOP("IcmpSelector", "Fragment Reassembly Time Exceeded") },
    { false, typeItemCode, 12, QT_TRANSLATE_NOOP("IcmpSelector", "Parameter Problem") },
    { false, 0, 12, QT_TRANSLATE_NOOP("IcmpSelector", "Pointer Indicates the Error") },
    { false, 1, 12, QT_TRANSLATE_NOOP("IcmpSelector", "Missing a Required Option") },
    { false, 2, 12, QT_TRANSLATE_NOOP("IcmpSelector", "Bad Length") },
    { false, typeItemCode, 13, QT_TRANSLATE_NOOP("IcmpSelector", "Timestamp Request") },
    { false, typeItemCode, 14, QT_TRANSLATE_NOOP("IcmpSelector", "Timestamp Reply") },
    // ICMPv6
    { true, typeItemCode, 1, QT_TRANSLATE_NOOP("IcmpSelector", "Destination Unreachable") },
    { true, 0, 1, QT_TRANSLATE_NOOP("IcmpSelector", "No Route") },
    { true, 1, 1, QT_TRANSLATE_NOOP("IcmpSelector", "Administratively Prohibited") },
    { true, 3, 1, QT_TRANSLATE_NOOP("IcmpSelector", "Address Unreachable") },
    { true, 4, 1, QT_TRANSLATE_NOOP("IcmpSelector", "Port Unreachable") },
    { true, typeItemCode, 2, QT_TRANSLATE_NOOP("IcmpSelector", "Packet Too Big") },
    { true, typeItemCode, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Time Exceeded") },
    { true, 0, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Hop Limit Exceeded in Transit") },
    { true, 1, 3, QT_TRANSLATE_NOOP("IcmpSelector", "Fragment Reassembly Time Exceeded") },
    { true, typeItemCode, 4, QT_TRANSLATE_NOOP("IcmpSelector", "Parameter Problem") },
    { true, 0, 4, QT_TRANSLATE_NOOP("IcmpSelector", "Erroneous Header Field") },
    { true, 1, 4, QT_TRANSLATE_NOOP("IcmpSelector", "Unrecognized Next Header") },
    { true, 2, 4, QT_TRANSLATE_NOOP("IcmpSelector", "Unrecognized IPv6 Option") },
    { true, typeItemCode, 128, QT_TRANSLATE_NOOP("IcmpSelector", "Echo Request") },
    { true, typeItemCode, 129, QT_TRANSLATE_NOOP("IcmpSelector", "Echo Reply") },
    { true, typeItemCode, 130, QT_TRANSLATE_NOOP("IcmpSelector", "Multicast Listener Query") },
    { true, typeItemCode, 131, QT_TRANSLATE_NOOP("IcmpSelector", "Multicast Listener Report") },
    { true, typeItemCode, 132, QT_TRANSLATE_NOOP("IcmpSelector", "Multicast Listener Done") },
    { true, typeItemCode, 133, QT_TRANSLATE_NOOP("IcmpSelector", "Router Solicitation") },
    { true, typeItemCode, 134, QT_TRANSLATE_NOOP("IcmpSelector", "Router Advertisement") },
    { true, typeItemCode, 135, QT_TRANSLATE_NOOP("IcmpSelector", "Neighbor Solicitation") },
    { true, typeItemCode, 136, QT_TRANSLATE_NOOP("IcmpSelector", "Neighbor Advertisement") },
    { true, typeItemCode, 137, QT_TRANSLATE_NOOP("IcmpSelector", "Redirect") },
    { true, typeItemCode, 143, QT_TRANSLATE_NOOP("IcmpSelector", "Multicast Listener Report v2") },
};

bool isItemVersion(const IcmpItem &item, bool isIcmp, bool isIcmp6)
{
    return item.isIcmp6 ? isIcmp6 : isIcmp;
}

// The type's item, else the type's code's one
bool isItemListed(const IcmpItem &item, bool isCodes, int type)
{
    if (!isCodes) {
        return item.code == typeItemCode;
    }

    return item.type == type && item.code != typeItemCode;
}

// e.g. "Echo Request", "Echo Request (ICMPv6)"
QString itemName(const IcmpItem &item, bool isBothVersions)
{
    const QString name = IcmpSelector::tr(item.name);

    return (isBothVersions && item.isIcmp6) ? name + " (ICMPv6)" : name;
}

// The same value's items are joined: it matches them all, e.g. ICMP's and ICMPv6's type 3
void addItem(IcmpSelector::ValuesList &values, QStringList &names, int value, const QString &name)
{
    const int index = values.indexOf(value);

    if (index < 0) {
        values << value;
        names << name;
        return;
    }

    names[index] += " / " + name;
}

}

IcmpSelector::IcmpSelector(QWidget *parent) : EditCombo(parent) { }

void IcmpSelector::setTypes(bool isIcmp, bool isIcmp6)
{
    setupItems(isIcmp, isIcmp6, /*isCodes=*/false);
}

void IcmpSelector::setCodes(bool isIcmp, bool isIcmp6, int type)
{
    setupItems(isIcmp, isIcmp6, /*isCodes=*/true, type);
}

void IcmpSelector::setupItems(bool isIcmp, bool isIcmp6, bool isCodes, int type)
{
    const bool isBothVersions = isIcmp && isIcmp6;

    ValuesList values = { customValue };
    QStringList names = { tr("Custom") };

    for (const IcmpItem &item : g_icmpItems) {
        if (!isItemVersion(item, isIcmp, isIcmp6) || !isItemListed(item, isCodes, type))
            continue;

        addItem(values, names, isCodes ? item.code : item.type, itemName(item, isBothVersions));
    }

    setItems(values, names);
}
