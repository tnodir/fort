#ifndef PROTOCOLSELECTOR_H
#define PROTOCOLSELECTOR_H

#include "spincombo.h"

// The protocol's number with the named values
class ProtocolSelector : public SpinCombo
{
    Q_OBJECT

public:
    explicit ProtocolSelector(bool hasAny = false, QWidget *parent = nullptr);

    // -1: Any
    int protocol() const;
    void setProtocol(int v);

    bool isAny() const;
    void setAny();

    void retranslateUi();

signals:
    void protocolChanged();

private:
    void setupUi(bool hasAny);
};

#endif // PROTOCOLSELECTOR_H
