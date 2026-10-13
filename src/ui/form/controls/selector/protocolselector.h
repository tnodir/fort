#ifndef PROTOCOLSELECTOR_H
#define PROTOCOLSELECTOR_H

#include <form/controls/spincombo.h>

// The protocol's number with the named values
class ProtocolSelector : public SpinCombo
{
    Q_OBJECT

public:
    explicit ProtocolSelector(QWidget *parent = nullptr);

    int protocol() const;
    void setProtocol(int v);

    void retranslateUi();

signals:
    void protocolChanged();

private:
    void setupUi();
};

#endif // PROTOCOLSELECTOR_H
