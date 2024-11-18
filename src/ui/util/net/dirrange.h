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

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    bool m_isIn : 1 = false;
    bool m_isOut : 1 = false;
};

#endif // DIRRANGE_H
