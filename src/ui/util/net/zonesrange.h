#ifndef ZONESRANGE_H
#define ZONESRANGE_H

#include <QObject>

#include "textrange.h"

class ZonesRange : public TextRange
{
    Q_OBJECT

public:
    explicit ZonesRange(QObject *parent = nullptr);

    bool isResult() const { return m_isResult; }
    bool isAccepted() const { return m_isAccepted; }
    bool isRejected() const { return m_isRejected; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    void write(ConfData &confData) const override;

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    bool m_isResult : 1 = false;
    bool m_isAccepted : 1 = false;
    bool m_isRejected : 1 = false;
};

#endif // ZONESRANGE_H
