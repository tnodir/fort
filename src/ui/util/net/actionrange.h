#ifndef ACTIONRANGE_H
#define ACTIONRANGE_H

#include <QObject>

#include "textrange.h"

class ActionRange : public TextRange
{
    Q_OBJECT

public:
    explicit ActionRange(QObject *parent = nullptr);

    quint8 actionTypeId() const { return m_actionTypeId; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    void write(ConfData &confData) const override;

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    quint8 m_actionTypeId = 0;
};

#endif // ACTIONRANGE_H
