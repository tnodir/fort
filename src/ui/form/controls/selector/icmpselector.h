#ifndef ICMPSELECTOR_H
#define ICMPSELECTOR_H

#include <form/controls/editcombo.h>

// The ICMP's and ICMPv6's types or a type's codes by their names
class IcmpSelector : public EditCombo
{
    Q_OBJECT

public:
    explicit IcmpSelector(QWidget *parent = nullptr);

    // The ICMP's or ICMPv6's types, or both
    void setTypes(bool isIcmp, bool isIcmp6);

    // The type's codes of ICMP or ICMPv6, or both; the type < 0 (not one type) has none
    void setCodes(bool isIcmp, bool isIcmp6, int type);

private:
    // The types or the type's codes of ICMP or ICMPv6, or both: then the ICMPv6's names are marked
    // and the names of the same value are joined
    void setupItems(bool isIcmp, bool isIcmp6, bool isCodes, int type = -1);
};

#endif // ICMPSELECTOR_H
