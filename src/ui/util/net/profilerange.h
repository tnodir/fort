#ifndef PROFILERANGE_H
#define PROFILERANGE_H

#include <QObject>

#include "textrange.h"

class ProfileRange : public TextRange
{
    Q_OBJECT

public:
    explicit ProfileRange(QObject *parent = nullptr);

    quint8 profileId() const { return m_profileId; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    void write(ConfData &confData) const override;

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    quint8 m_profileId = 0;
};

#endif // PROFILERANGE_H
