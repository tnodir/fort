#ifndef AREARANGE_H
#define AREARANGE_H

#include <QObject>

#include "textrange.h"

class AreaRange : public TextRange
{
    Q_OBJECT

public:
    explicit AreaRange(QObject *parent = nullptr);

    bool isLocalhost() const { return m_isLocalhost; }
    bool isLan() const { return m_isLan; }
    bool isInet() const { return m_isInet; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

protected:
    TextRange::ParseError parseText(const QStringView &text);

private:
    bool m_isLocalhost : 1 = false;
    bool m_isLan : 1 = false;
    bool m_isInet : 1 = false;
};

#endif // AREARANGE_H
