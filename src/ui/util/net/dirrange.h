#ifndef DIRRANGE_H
#define DIRRANGE_H

#include <QObject>

#include "valuerange.h"

class DirRange : public ValueRange
{
    Q_OBJECT

public:
    explicit DirRange(QObject *parent = nullptr);

    bool isIn() const { return m_isIn; }
    bool isOut() const { return m_isOut; }

    bool isEmpty() const override;

    void clear() override;

    QString toText() const override;

    bool fromList(const StringViewList &list, bool sort = true) override;

private:
    enum ParseError {
        ErrorOk = 0,
        ErrorBadDirection,
    };

    DirRange::ParseError parseDirectionLine(const QStringView &line);

private:
    bool m_isIn : 1 = false;
    bool m_isOut : 1 = false;
};

#endif // DIRRANGE_H
