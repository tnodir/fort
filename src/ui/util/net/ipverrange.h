#ifndef IPVERRANGE_H
#define IPVERRANGE_H

#include <QObject>

#include "textrange.h"

class IpVerRange : public TextRange
{
    Q_OBJECT

public:
    explicit IpVerRange(QObject *parent = nullptr);

    bool isV4() const { return m_isV4; }
    bool isV6() const { return m_isV6; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    void write(ConfData &confData) const override;

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    bool m_isV4 : 1 = false;
    bool m_isV6 : 1 = false;
};

#endif // IPVERRANGE_H
