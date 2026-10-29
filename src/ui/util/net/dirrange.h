#ifndef DIRRANGE_H
#define DIRRANGE_H

#include <QObject>

#include "textrange.h"

class DirRange : public TextRange
{
    Q_OBJECT

public:
    explicit DirRange(QObject *parent = nullptr);

    bool isIn() const { return m_isIn; }
    bool isOut() const { return m_isOut; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    /* The direction's name in the rule text */
    static QString directionName(bool inbound);

    /* The direction's id by its name, 0 - unknown */
    static quint8 directionIdByName(const QString &name);

    void write(ConfData &confData) const override;

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    bool m_isIn : 1 = false;
    bool m_isOut : 1 = false;
};

#endif // DIRRANGE_H
